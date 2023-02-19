/******************************************************************************
 * window.h
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2023 - present
 * Last modified February 2023
 * ---------------------------------------------------------------------------
 * Creates and manages the window that displays visuals to the user.
 * Also (partially) responsible for managing user input.
 *****************************************************************************/
#ifndef WC_SYS_WINDOW_H
#define WC_SYS_WINDOW_H

namespace wc::window {

  bool init();
  void shutdown();

  bool handleMessages();

  void getWindowSize(int& w, int& h);
  void getDrawableSize(int& w, int& h);

}  // namespace wc::window

#endif  // WC_SYS_WINDOW_H