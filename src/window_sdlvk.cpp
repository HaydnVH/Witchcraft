#ifdef PLATFORM_SDL
#ifdef RENDERER_VULKAN
#include <SDL.h>
#include <SDL_vulkan.h>

#include "appconfig.h"
#include "config.h"
#include "debug.h"

namespace wc {
namespace window {

	namespace {

		SDL_Window* _window = nullptr;

		struct {

			int width = 1280, height = 720;
			bool maximized = false;
			bool fs_enabled = false, fs_borderless_window = true;
			int fs_width = 0, fs_height = 0;

			bool modified = false;

		} _config;

	} // namespace <anon>

	bool init() {

		if (!config::isInitialized()) {
			debug::fatal("Can't open window before loading config!\n");
		}

		// Load in config settings.
		if (config::exists("window")) {
			config::read("window", "width", _config.width);
			config::read("window", "height", _config.height);
			config::read("window", "maximized", _config.maximized);
			config::read("window.fullscreen", "enabled", _config.fs_enabled);
			config::read("window.fullscreen", "borderless window", _config.fs_borderless_window);
			config::read("window.fullscreen", "width", _config.fs_width);
			config::read("window.fullscreen", "height", _config.fs_height);
		}
		else _config.modified = true;

		// Initialize SDL.
		SDL_Init(SDL_INIT_VIDEO);

		// Determine window flags.
		uint32_t flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN;
		if (_config.maximized) { flags |= SDL_WINDOW_MAXIMIZED; }

		// Create the widnow.
		_window = SDL_CreateWindow(
			appconfig::getAppName().c_str(),
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			_config.width, _config.height,
			flags
		);
		if (_window == nullptr) {
			debug::fatal("Failed to create window.\n");
			return false;
		}

		debug::info("Window opened.\n");
		debug::infomore(_config.width, " x ", _config.height, "\n");
		return true;
	}

	void shutdown() {
		if (_window) {
			SDL_DestroyWindow(_window);
		}

		// Save config settings.
		if (_config.modified) {
			config::write("window", "width", _config.width);
			config::write("window", "height", _config.height);
			config::write("window", "maximized", _config.maximized);
			config::write("window.fullscreen", "enabled", _config.fs_enabled);
			config::write("window.fullscreen", "borderless window", _config.fs_borderless_window);
			config::write("window.fullscreen", "width", _config.fs_width);
			config::write("window.fullscreen", "height", _config.fs_height);
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
					_config.modified = true;
					_config.maximized = SDL_GetWindowFlags(_window) & SDL_WINDOW_MAXIMIZED;
					if (!_config.maximized) {
						_config.width = _event.window.data1;
						_config.height = _event.window.data2;
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

}} // namespace wc::window


#endif
#endif