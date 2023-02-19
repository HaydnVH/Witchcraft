/******************************************************************************
 * main.cpp
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2022 - present
 * Last modified December 2022
 * ---------------------------------------------------------------------------
 * Defines the entrypoint into the application.
 *****************************************************************************/

#include "filesys/vfs.h"
#include "lua/luasystem.h"
#include "sys/appconfig.h"
#include "sys/cli.h"
#include "sys/debug.h"
#include "sys/mainloop.h"
#include "sys/paths.h"
#include "sys/window.h"

#include <iostream>

namespace {

  /// Initialize services and subsystems.
  /// @return
  ///   0 on success, otherwise a unique integer
  ///   depending on which subsystem failed to initialize.
  int startup() {
    cli::initialize();
    {
      dbg::info(
          {fmt::format("Now starting '{}' {}", wc::APP_NAME, wc::APP_VERSION),
           fmt::format("Created using '{}' {}", wc::ENGINE_NAME,
                       wc::ENGINE_VERSION),
           "¯\\_(ツ)_/¯ 🌮"});
    }

    // userconfig::init();
    wc::lua::init();
    if (!wc::vfs::init()) return 20;
    if (!wc::window::init()) return 30;
    // if (!gfx::init()) return 40;

    // entity::initLua();

    return 0;
  }

  /// Shuts down services and subsystems.
  /// @param startupResult: The value returned by 'startup()'.
  void shutdown(int startupResult) {

    // gfx::shutdown();
    wc::window::shutdown();
    wc::vfs::shutdown();
    // cli::showCrashReports();
    cli::shutdown();
    // userconfig::shutdown();
  }

}  // namespace

/// The main entrypoint into the application.
/// @param argc: The number of arguments passed to the application (unused).
/// @param argv: The array of arguments passed to the application (unused).
/// @return Standard application result code.  0 on success.
int main(int /*argc*/, char** /*argv*/) {
  try {
    // Initialize services and subsystems.
    int result = ::startup();

    // Enter the main loop.
    if (result == 0) {
      while (wc::sys::isRunning()) { wc::sys::mainLoop(true); }
    }

    // Shutdown services and subsystems.
    ::shutdown(result);
    return result;
  } catch (const std::exception& e) {
    std::cout << "\n" << e.what() << std::endl;
    return 1;
  }
}
