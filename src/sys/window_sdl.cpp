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
    } fullscreen;
    bool modified = false;
  } settings;

}  // namespace

bool wc::window::init() {

  if (window_s) {
    dbg::fatal("Can't open the window a second time!");
    return false;
  }

  // TODO: Load settings

  SDL_Init(SDL_INIT_VIDEO);

  uint32_t flags = SDL_WINDOW_RESIZABLE;
  #ifdef RENDERER_VULKAN
  flags |= SDL_WINDOW_VULKAN;
  #endif
  if (settings.bMaximized) { flags |= SDL_WINDOW_MAXIMIZED; }
  window_s = SDL_CreateWindow(wc::APP_NAME, SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, settings.iWidth,
                              settings.iHeight, flags);
  if (!window_s) {
    dbg::fatal({"Failed to create window.", SDL_GetError()});
    return false;
  }

  dbg::info(fmt::format("Window opened; dimensions {} x {}.", settings.iWidth,
                        settings.iHeight));
  return true;
}

void wc::window::shutdown() {
  if (window_s) SDL_DestroyWindow(window_s);

  // TODO: Save settings.
}

bool wc::window::handleMessages() {
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    switch (e.type) {
    case SDL_QUIT: return false;
    case SDL_WINDOWEVENT: {
      switch (e.window.event) {
      case SDL_WINDOWEVENT_RESIZED:
        settings.modified = true;
        settings.bMaximized =
            SDL_GetWindowFlags(window_s) & SDL_WINDOW_MAXIMIZED;
        if (!settings.bMaximized) {
          settings.iWidth  = e.window.data1;
          settings.iHeight = e.window.data2;
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