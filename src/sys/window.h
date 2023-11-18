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

#include "settings.h"

#ifdef PLATFORM_SDL
#include "SDL2/SDL.h"
#endif

namespace wc {

  class Window {
  public:
    Window(wc::SettingsFile& settings);
    ~Window();

    bool handleMessages();

    void getWindowSize(int& w, int& h);
    void getDrawableSize(int& w, int& h);

#ifdef PLATFORM_SDL
    SDL_Window* getHandle() { return window_; }
#endif  // PLATFORM_SDL

  private:
    wc::SettingsFile& settingsFile_;

    // This struct holds everything that can be read and saved via
    // settings.json.
    struct Settings {
      int  iWidth = 1280, iHeight = 720;
      bool bMaximized = false;
      struct Fullscreen {
        bool bEnabled    = false;
        bool bBorderless = true;
        int  iWidth = 0, iHeight = 0;

        bool operator==(const Fullscreen&) const = default;
      } fs;

      bool operator==(const Settings&) const = default;
    };

    Window::Settings initialSettings_;
    Window::Settings settings_;

#ifdef PLATFORM_SDL
    SDL_Window* window_;
#endif
  };

}  // namespace wc

#endif  // WC_SYS_WINDOW_H