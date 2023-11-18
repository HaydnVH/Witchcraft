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
#include "dbg/debug.h"
#include "settings.h"
#include "window.h"

#include <SDL2/SDL.h>
#ifdef RENDERER_VULKAN
#include <SDL2/SDL_vulkan.h>
#endif

namespace {
  wc::Window* uniqueWindow_s = nullptr;
}

wc::Window::Window(wc::SettingsFile& settingsFile):
    settingsFile_(settingsFile) {

  // Uniqueness check
  if (uniqueWindow_s)
    throw dbg::Exception("Only one wc::Window object should exist.");
  uniqueWindow_s = this;

  // Read settings.
  settingsFile_.read("Window.iWidth", settings_.iWidth);
  settingsFile_.read("Window.iHeight", settings_.iHeight);
  settingsFile_.read("Window.bMaximized", settings_.bMaximized);
  settingsFile_.read("Window.Fullscreen.bEnabled", settings_.fs.bEnabled);
  settingsFile_.read("Window.Fullscreen.bBorderless", settings_.fs.bBorderless);
  settingsFile_.read("Window.Fullscreen.iWidth", settings_.fs.iWidth);
  settingsFile_.read("Window.Fullscreen.iHeight", settings_.fs.iHeight);
  initialSettings_ = settings_;

  SDL_Init(SDL_INIT_VIDEO);

  // Create the window.
  uint32_t flags = SDL_WINDOW_RESIZABLE;
#ifdef RENDERER_VULKAN
  flags |= SDL_WINDOW_VULKAN;
#endif
  if (settings_.bMaximized) {
    flags |= SDL_WINDOW_MAXIMIZED;
  }
  window_ = SDL_CreateWindow(wc::APP_NAME, SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED, settings_.iWidth,
                             settings_.iHeight, flags);
  if (!window_) {
    throw dbg::Exception(
        std::format("Failed to create window. {}", SDL_GetError()));
  }

  dbg::info(std::format("Window opened; dimensions {} x {}.", settings_.iWidth,
                        settings_.iHeight));
}

wc::Window::~Window() {
  if (window_)
    SDL_DestroyWindow(window_);

  // If settings have changed since load, we'll have to save them.
  if (settings_ != initialSettings_) {
    settingsFile_.write("Window.iWidth", settings_.iWidth);
    settingsFile_.write("Window.iHeight", settings_.iHeight);
    settingsFile_.write("Window.bMaximized", settings_.bMaximized);
    settingsFile_.write("Window.Fullscreen.bEnabled", settings_.fs.bEnabled);
    settingsFile_.write("Window.Fullscreen.bBorderless",
                        settings_.fs.bBorderless);
    settingsFile_.write("Window.Fullscreen.iWidth", settings_.fs.iWidth);
    settingsFile_.write("Window.Fullscreen.iHeight", settings_.fs.iHeight);
  }
}

bool wc::Window::handleMessages() {
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    switch (e.type) {
    case SDL_QUIT: return false;
    case SDL_WINDOWEVENT: {
      switch (e.window.event) {
      case SDL_WINDOWEVENT_RESIZED:
        settings_.bMaximized =
            SDL_GetWindowFlags(window_) & SDL_WINDOW_MAXIMIZED;
        if (!settings_.bMaximized) {
          settings_.iWidth  = e.window.data1;
          settings_.iHeight = e.window.data2;
        }
        dbg::infomore(std::format("Window resized to {} x {}", e.window.data1,
                                  e.window.data2));
        break;
      }
    } break;
    }
  }
  return true;
}

void wc::Window::getWindowSize(int& w, int& h) {}
void wc::Window::getDrawableSize(int& w, int& h) {
#ifdef RENDERER_VULKAN
  SDL_Vulkan_GetDrawableSize(window_, &w, &h);
#endif
}

#endif  // PLATFORM_SDL