/******************************************************************************
 * window_sdl.cpp
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2023 - present
 * Last modified February 2023
 * ---------------------------------------------------------------------------
 * Implementation for the window system on SDL-based platforms.
 *****************************************************************************/
#ifdef PLATFORM_SDL

  #include "appconfig.h"
  #include "debug.h"
  #include "settings.h"
  #include "window.h"

  #include <SDL.h>

namespace {
  SDL_Window* window_s = nullptr;

  struct {
    int  iWidth = 1280, iHeight = 720;
    bool bMaximized = false;
    struct {
      bool bEnabled    = false;
      bool bBorderless = true;
      int  iWidth = 0, iHeight = 0;
    } fs;
    bool modified = false;
  } settings_s;

}  // namespace

bool wc::window::init() {

  if (window_s) {
    dbg::fatal("Can't open the window a second time!");
    return false;
  }

  wc::settings::read("Window.iWidth", settings_s.iWidth);
  wc::settings::read("Window.iHeight", settings_s.iHeight);
  wc::settings::read("Window.bMaximized", settings_s.bMaximized);
  wc::settings::read("Window.Fullscreen.bEnabled", settings_s.fs.bEnabled);
  wc::settings::read("Window.Fullscreen.bBorderless",
                     settings_s.fs.bBorderless);
  wc::settings::read("Window.Fullscreen.iWidth", settings_s.fs.iWidth);
  wc::settings::read("Window.Fullscreen.iHeight", settings_s.fs.iHeight);

  SDL_Init(SDL_INIT_VIDEO);

  uint32_t flags = SDL_WINDOW_RESIZABLE;
  #ifdef RENDERER_VULKAN
  flags |= SDL_WINDOW_VULKAN;
  #endif
  if (settings_s.bMaximized) { flags |= SDL_WINDOW_MAXIMIZED; }
  window_s = SDL_CreateWindow(wc::APP_NAME, SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, settings_s.iWidth,
                              settings_s.iHeight, flags);
  if (!window_s) {
    dbg::fatal({"Failed to create window.", SDL_GetError()});
    return false;
  }

  dbg::info(fmt::format("Window opened; dimensions {} x {}.", settings_s.iWidth,
                        settings_s.iHeight));
  return true;
}

void wc::window::shutdown() {
  if (window_s) SDL_DestroyWindow(window_s);

  wc::settings::write("Window.iWidth", settings_s.iWidth);
  wc::settings::write("Window.iHeight", settings_s.iHeight);
  wc::settings::write("Window.bMaximized", settings_s.bMaximized);
  wc::settings::write("Window.Fullscreen.bEnabled", settings_s.fs.bEnabled);
  wc::settings::write("Window.Fullscreen.bBorderless",
                      settings_s.fs.bBorderless);
  wc::settings::write("Window.Fullscreen.iWidth", settings_s.fs.iWidth);
  wc::settings::write("Window.Fullscreen.iHeight", settings_s.fs.iHeight);
}

bool wc::window::handleMessages() {
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    switch (e.type) {
    case SDL_QUIT: return false;
    case SDL_WINDOWEVENT: {
      switch (e.window.event) {
      case SDL_WINDOWEVENT_RESIZED:
        settings_s.modified = true;
        settings_s.bMaximized =
            SDL_GetWindowFlags(window_s) & SDL_WINDOW_MAXIMIZED;
        if (!settings_s.bMaximized) {
          settings_s.iWidth  = e.window.data1;
          settings_s.iHeight = e.window.data2;
        }
        dbg::infomore(fmt::format("Window resized to {} x {}", e.window.data1,
                                  e.window.data2));
        break;
      }
    } break;
    }
  }
  return true;
}

void wc::window::getWindowSize(int& w, int& h) {}
void wc::window::getDrawableSize(int& w, int& h) {}

#endif  // PLATFORM_SDL