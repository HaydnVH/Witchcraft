/******************************************************************************
 * main.cpp
 * Part of the Witchcraft engine by Haydn V. Harach
 * (C) 2022 - present
 ******************************************************************************
 * Defines the entrypoint into the application.
 *****************************************************************************/

namespace wc::sys {

	/// Initialize services and subsystems.
	/// Returns:
	///   0 on success, otherwise a unique integer
	///   depending on which subsystem failed to initialize.
	int startup() {
		//debug::info("Now starting ", appconfig::AppName, " ", appconfig::AppVer, ".\n");
		//debug::info("Created using ", appconfig::EngineName, " ", appconfig::EngineVer, ".\n");

		//if (!wc::initPaths()) return 10;
		//userconfig::init();
		//debug::init(wc::getUserPath().string().c_str());
		//lua::init();
		//if (!vfs::init()) return 20;
		//if (!window::init()) return 30;
		//if (!gfx::init()) return 40;

		//entity::initLua();

		return 0;
	}

	/// Shuts down services and subsystems.
	/// Parameters:
	///   startupResult: The value returned by 'startup'.
	void shutdown(int startupResult) {
		//gfx::shutdown();
		//window::shutdown();
		//vfs::shutdown();
		//debug::showCrashReports(); debug::shutdown();
		//userconfig::shutdown();
	}

} // namespace wc::sys

/// The main entrypoint into the application.
/// Parameters:
///   argc: The number of arguments passed to the application.
///   argv: The array of arguments passed to the application.
/// Returns:
///   Standard application result code.  0 on success.
int main(int /*argc*/, char** /*argv*/) {
	// Initialize services and subsystems.
	int result = wc::sys::startup();

	// Enter the main loop.
	if (result == 0) {
		while (wc::running) {
			wc::mainloop(true);
		}
	}

	// Shutdown services and subsystems.
	wc::sys::shutdown();
	return result;
}