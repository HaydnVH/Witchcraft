/******************************************************************************
 * main.cpp
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2022 - present
 * Last modified December 2022
 * ---------------------------------------------------------------------------
 * Defines the entrypoint into the application.
 *****************************************************************************/

#include "dbg/cli.h"
#include "dbg/debug.h"
#include "filesys/vfs.h"
#include "gfx/renderer_vk.h"
#include "lua/luasystem.h"
#include "sys/appconfig.h"
#include "sys/mainloop.h"
#include "sys/paths.h"
#include "sys/settings.h"
#include "sys/window.h"
#include "tools/uuid.h"

#include <chrono>
#include <iostream>
#include <unordered_set>

/// The main entrypoint into the application.
/// @param argc: The number of arguments passed to the application (unused).
/// @param argv: The array of arguments passed to the application (unused).
/// @return Standard application result code.  0 on success.
int main(int /*argc*/, char** /*argv*/) {
  int result = 0;
  // try {
  cli::init();
  try {
    dbg::info(
        fmt::format("Now starting '{}' {}\n"
                    "Created using '{}' {}\n"
                    "Unicode handling test: ¯\\_(ツ)_/¯ 🌮 {:x}\n",
                    wc::APP_NAME, wc::APP_VERSION, wc::ENGINE_NAME,
                    wc::ENGINE_VERSION, 42));

    wc::SettingsFile  settings("settings.json");
    wc::Filesystem    vfs;
    wc::Lua           lua(vfs);
    wc::Window        window(settings);
    wc::gfx::Renderer renderer(settings, window);

    // Enter the main loop.
    while (wc::sys::isRunning()) {
      wc::sys::mainLoop(lua, window, true);
    }

  } catch (const dbg::Exception& e) {
    dbg::fatal(e);
    result = 10;
  }
  cli::shutdown();
  //}
  // catch (const std::exception& e) {
  //  std::cerr << "\n" << e.what() << std::endl;
  //  result = 20;
  //}
  return result;
}
