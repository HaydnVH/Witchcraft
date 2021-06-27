#include <iostream>
using namespace std;

#include "appconfig.h"
#include "debug.h"
#include "sys/window.h"
#include "events.h"

int startup() {
	appconfig::Init();
	debug::Init();
	if (!sys::window::Init()) return 10;

	return 0;
}

void shutdown() {
	sys::window::Shutdown();
	debug::showCrashReports(); debug::Shutdown();
}

int main(int argc, char* argv[]) {
	// Initialize services and subsystems.
	int result = startup();

	// Handle the window's message pump as we enter the main loop.
	if (result == 0) {
		while (sys::window::HandleMessages())
		{
			// Handle terminal input.
			std::string terminal_input;
			while (debug::popInput(terminal_input)) {
				debug::user(terminal_input, "\n");
				//lua::runString(console_input.c_str(), "CONSOLE", "@Console");
			}

			events::earlyLogicalUpdate().Execute();
			events::onLogicalUpdate().Execute();
			events::lateLogicalUpdate().Execute();
		}
	}

	// Shutdown services and subsystems.
	shutdown();
	return result;
}