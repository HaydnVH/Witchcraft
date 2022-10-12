#ifdef PLATFORM_SDL
#ifdef RENDERER_VULKAN
#include <SDL.h>
#include <SDL_vulkan.h>

#include "appconfig.h"
#include "userconfig.h"
#include "debug.h"

namespace wc {
namespace window {

	namespace {

		SDL_Window* pWindow = nullptr;

		struct {

			int iWidth = 1280, iHeight = 720;
			bool bMaximized = false;

			struct {
				bool bEnabled = false;
				bool bBorderless = true;
				int iWidth = 0, iHeight = 0;
			} fs;
			bool modified = false;

		} config;

	} // namespace <anon>

	bool init() {

		if (!userconfig::isInitialized()) {
			debug::fatal("Can't open window before loading user config!\n");
			return false;
		}

		if (pWindow) {
			debug::fatal("Can't open the window a second time!\n");
			return false;
		}

		// Load in config settings.
		if (userconfig::exists("window")) {
			userconfig::read("window", "iWidth", config.iWidth);
			userconfig::read("window", "iHeight", config.iHeight);
			userconfig::read("window", "bMaximized", config.bMaximized);
			userconfig::read("window.fullscreen", "bEnabled", config.fs.bEnabled);
			userconfig::read("window.fullscreen", "bBorderless", config.fs.bBorderless);
			userconfig::read("window.fullscreen", "iWidth", config.fs.iWidth);
			userconfig::read("window.fullscreen", "iHeight", config.fs.iHeight);
		}
		else config.modified = true;

		// Initialize SDL.
		SDL_Init(SDL_INIT_VIDEO);

		// Determine window flags.
		uint32_t flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN;
		if (config.bMaximized) { flags |= SDL_WINDOW_MAXIMIZED; }

		// Create the widnow.
		pWindow = SDL_CreateWindow(
			appconfig::APP_NAME,
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			config.iWidth, config.iHeight,
			flags
		);
		if (pWindow == nullptr) {
			debug::fatal("Failed to create window.\n");
			return false;
		}

		debug::info("Window opened.\n");
		debug::infomore(config.iWidth, " x ", config.iHeight, "\n");
		return true;
	}

	void shutdown() {
		if (pWindow) {
			SDL_DestroyWindow(pWindow);
		}

		// Save config settings.
		if (config.modified) {
			userconfig::write("window", "iWidth", config.iWidth);
			userconfig::write("window", "iHeight", config.iHeight);
			userconfig::write("window", "bMaximized", config.bMaximized);
			userconfig::write("window.fullscreen", "bEnabled", config.fs.bEnabled);
			userconfig::write("window.fullscreen", "bBorderless", config.fs.bBorderless);
			userconfig::write("window.fullscreen", "iWidth", config.fs.iWidth);
			userconfig::write("window.fullscreen", "iHeight", config.fs.iHeight);
		}
	}

	bool handleMessages() {
		SDL_Event _event;
		while (SDL_PollEvent(&_event) != 0) {
			switch (_event.type) {
			case SDL_QUIT:
				return false;
				break;
			case SDL_WINDOWEVENT: {
				switch (_event.window.event) {
				case SDL_WINDOWEVENT_RESIZED:
					config.modified = true;
					config.bMaximized = SDL_GetWindowFlags(pWindow) & SDL_WINDOW_MAXIMIZED;
					if (!config.bMaximized) {
						config.iWidth = _event.window.data1;
						config.iHeight = _event.window.data2;
					}
					debug::info("Window resized to ", _event.window.data1, " x ", _event.window.data2, '\n');
					break;
				}
				} break;
			}
		}

		return true;
	}

	void getWindowSize(int& w, int& h) {}
	void getDrawableSize(int& w, int& h) {}

	SDL_Window* getHandle() { return pWindow; }

}} // namespace wc::window


#endif
#endif