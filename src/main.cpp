#include <iostream>
using namespace std;

#include "appconfig.h"
#include "debug.h"
#include "sys/window.h"

int main(int argc, char* argv[]) {
	// Initialize services and subsystems.
	appconfig::Init();
	debug::Init(); debug::info("Hello World\n");
	if (!sys::window::Init()) return 10;

	// Handle the window's message pump as we enter the main loop.
	while (sys::window::HandleMessages())
	{
		// Handle terminal input.
		std::string terminal_input;
		while (debug::popInput(terminal_input)) {
			debug::user(terminal_input, "\n");
			//lua::runString(console_input.c_str(), "CONSOLE", "@Console");
		}
	}

	// Shutdown services and subsystems.
	sys::window::Shutdown();
	debug::showCrashReports(); debug::Shutdown();
	return 0;
}