#include "main.h"

#include <iostream>
using namespace std;

#include "appconfig.h"
#include "debug.h"
#include "window.h"
#include "events.h"
#include "lua/luasystem.h"

namespace {
	
	// Initialize subsystems.
	int startup() {
		wc::appconfig::Init();
		debug::Init(wc::appconfig::getUserDir().c_str());
		wc::lua::Init();
		if (!wc::window::Init()) return 10;

		return 0;
	}

	// Shut down subsystems.
	void shutdown() {
		wc::window::Shutdown();
		debug::showCrashReports(); debug::Shutdown();
	}

	bool running = true;

} // namespace <anon>

namespace wc {

	void stop() {
		running = false;
	}

	void mainloop(bool handleWindowMessages) {

		// Handle window messages.
		if (handleWindowMessages) {
			running = window::HandleMessages();
			if (!running) { return; }
		}

		// Handle terminal input.
		std::string terminal_input;
		while (debug::popInput(terminal_input)) {
			debug::user(terminal_input, "\n");
			lua::RunString(terminal_input.c_str(), "CONSOLE", "@CommandLine");// , "CONSOLE", "@Console");
		}

		// Run OnLogicalUpdate events.
		events::earlyLogicalUpdate().Execute();
		events::onLogicalUpdate().Execute();
		events::lateLogicalUpdate().Execute();
	}


}; // namespace wc


///////////////////////////////////////////////////////////////////////////////
// Main entry point into the program.
///////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
	// Initialize services and subsystems.
	int result = startup();

	// Enter the main loop.
	if (result == 0) {
		while (running) {
			wc::mainloop(true);
	}	}

	// Shutdown services and subsystems.
	shutdown();
	return result;
}

