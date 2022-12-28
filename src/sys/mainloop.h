/******************************************************************************
 * mainloop.h
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2022 - present
 ******************************************************************************
 * Provides access to the main loop and related facilities.
 *****************************************************************************/
#ifndef WC_SYS_MAINLOOP_H
#define WC_SYS_MAINLOOP_H

namespace wc::sys {

  /// Checks whether the engine should be running.
  bool isRunning();

  /// Sets the internal 'running' flag to 'false' and allows the loop to terminate gracefully.
  void shutDown();

  /// Runs one iteration of the main loop.  Normally run in a tight loop.
  /// @param handleWindowMessages: Whether this iteration of the main loop
  /// should handle OS window messages.  Should only be false in special cases.
  void mainLoop(bool handleWindowMessages = true);

} // namespace wc::sys

#endif // WC_SYS_MAINLOOP_H