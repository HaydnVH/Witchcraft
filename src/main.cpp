#include "main.h"

#include <iostream>
#include <chrono>
using namespace std;

#include "appconfig.h"
#include "config.h"
#include "debug.h"
#include "window.h"
#include "events.h"
#include "lua/luasystem.h"
#include "filesys/vfs.h"


namespace wc {

	namespace {

		uint32_t logical_frame_counter = 0;
		uint32_t display_frame_counter = 0;
		double logical_time = 0.0;
		double display_time = 0.0;
		double logical_time_budget = 0.0;

		auto nowtime = std::chrono::high_resolution_clock::now();
		auto prevtime = nowtime;

		// Initialize subsystems.
		int startup() {
			appconfig::init();
			config::init();
			debug::init(appconfig::getUserDir().c_str());
			lua::init();
			if (!vfs::init()) return 10;
			if (!window::init()) return 20;

			return 0;
		}

		// Shut down subsystems.
		void shutdown() {
			window::shutdown();
			vfs::shutdown();
			debug::showCrashReports(); debug::shutdown();
			config::shutdown();
		}

		bool running = true;

	} // namespace <anon>

	void stop() {
		running = false;
	}

	void mainloop(bool handleWindowMessages) {

		// Get the amount of time passed since the previous call to mainLoop().
		prevtime = nowtime;
		nowtime = std::chrono::high_resolution_clock::now();
		double delta_time = std::chrono::duration<double>(nowtime - prevtime).count();

		// Check here for a very large delta_time.
		// We don't want to simulate too many frames at once.
		// This value might need to change depending on behaviour and performance.
		// In a multiplayer environment, this would trigger a state resync.
		if (delta_time > 1.0) { delta_time = 1.0; }

		// Add delta_time to the time budget, and run the appropriate number of logical updates.
		logical_time_budget += delta_time;
		while (logical_time_budget >= LOGICAL_SECONDS_PER_FRAME) {

			// Handle window messages.
			if (handleWindowMessages) {
				if (!window::handleMessages()) {
					running = false;
					return;
				}
			}

			// Handle terminal input.
			std::string terminal_input;
			while (debug::popInput(terminal_input)) {
				debug::user(terminal_input, "\n");
				lua::runString(terminal_input.c_str(), "CONSOLE", "@CommandLine");
			}

			// Run onLogicalUpdate events.
			events::earlyLogicalUpdate().Execute();
			events::onLogicalUpdate().Execute();
			events::lateLogicalUpdate().Execute();

			++logical_frame_counter;
			logical_time += LOGICAL_SECONDS_PER_FRAME;
			logical_time_budget -= LOGICAL_SECONDS_PER_FRAME;
		}

		// Run onDisplayUpdate events.
		// Draw the frame.
	}


}; // namespace wc


///////////////////////////////////////////////////////////////////////////////
// Main entry point into the program.
///////////////////////////////////////////////////////////////////////////////



int main(int argc, char* argv[]) {
	// Initialize services and subsystems.
	int result = wc::startup();

	// Enter the main loop.
	if (result == 0) {
		while (wc::running) {
			wc::mainloop(true);
	}	}

	// Shutdown services and subsystems.
	wc::shutdown();
	return result;
}

