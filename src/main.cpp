#include <iostream>
using namespace std;

#include "appconfig.h"
#include "debug.h"
#include "sys/window.h"
#include "events.h"

namespace {

	bool running = true;

} // namespace <anon>

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

void mainloop(bool handleWindowMessages = true) {

	// Handle window messages.
	if (handleWindowMessages) {
		running = sys::window::HandleMessages();
		if (!running) { return; }
	}

	// Handle terminal input.
	std::string terminal_input;
	while (debug::popInput(terminal_input)) {
		debug::user(terminal_input, "\n");
		// lua::runString(terminal_input.c_str(), "CONSOLE", "@Console");
	}

	// Run OnLogicalUpdate events.
	events::earlyLogicalUpdate().Execute();
	events::onLogicalUpdate().Execute();
	events::lateLogicalUpdate().Execute();
}

int main(int argc, char* argv[]) {
	// Initialize services and subsystems.
	int result = startup();

	// Enter the main loop.
	if (result == 0) {
		while (running) {
			mainloop(true);
	}	}

	// Shutdown services and subsystems.
	shutdown();
	return result;
}

