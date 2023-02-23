/******************************************************************************
 * mainloop.cpp
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2022 - present
 * Last modified December 2022
 * ---------------------------------------------------------------------------
 * Implements the main loop of the application.
 *****************************************************************************/
#include "mainloop.h"

#include "cli.h"
#include "debug.h"
#include "lua/luasystem.h"
#include "window.h"

#include <string>

namespace {
  bool running_s = true;
}

namespace wc::sys {

  bool isRunning() { return running_s; }
  void shutDown() { running_s = false; }

  void mainLoop(wc::Lua& lua, wc::Window& window, bool handleWindowMessages) {

    if (handleWindowMessages) {
      running_s = window.handleMessages();
    }

    // Handle terminal input.
    std::string terminalInput;
    while (cli::popInput(terminalInput)) {
      // if (terminalInput == "quit") { shutDown(); }
      lua.runString(terminalInput.c_str(), "CONSOLE", "@CLI");
    }
  }

}  // namespace wc::sys