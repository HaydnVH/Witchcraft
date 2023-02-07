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

#include <string>
#include "cli.h"
#include "debug.h"

namespace {
  bool running_s = true;
}

namespace wc::sys {

  bool isRunning() { return running_s; }
  void shutDown() { running_s = false; }

  void mainLoop(bool /*handleWindowMessages*/) {

    // Handle terminal input.
    std::string terminalInput;
    while (cli::popInput(terminalInput)) {
      if (terminalInput == "quit") { shutDown(); }
      //lua::runString(terminal_input.c_str(), "CONSOLE", "@CLI");
    }
  }

} // namespace wc::sys