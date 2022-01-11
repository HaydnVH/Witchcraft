#ifdef PLATFORM_SDL
#ifdef RENDERER_VULKAN
#include <SDL.h>
#include <SDL_vulkan.h>

#include "appconfig.h"
#include "debug.h"

namespace wc {
namespace window {

	namespace {

		SDL_Window* _window = nullptr;

		namespace _config {

			int width = 1280, height = 720;
			bool maximized = false;
			bool fs_enabled = false, fs_borderless_window = true;
			int fs_width = 0, fs_height = 0;

			bool modified = false;

		} // namespace _config

	} // namespace <anon>

	bool init() {
		SDL_Init(SDL_INIT_VIDEO);
		_window = SDL_CreateWindow(
			appconfig::getAppName().c_str(),
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			800, 600,
			SDL_WINDOW_VULKAN
		);
		if (_window == nullptr) {
			debug::fatal("In sys::window::Init():\n");
			debug::errmore("Failed to create window.\n");
			return false;
		}

		debug::info("Window opened.\n");
		debug::infomore("800 x 600\n");
		return true;
	}

	void shutdown() {
		if (_window) {
			SDL_DestroyWindow(_window);
		}
	}

	bool handleMessages() {
		SDL_Event _event;
		while (SDL_PollEvent(&_event) != 0) {
			switch (_event.type) {
			case SDL_QUIT:
				return false;
			}
		}

		return true;
	}

	void getWindowSize(int& w, int& h) {}
	void getDrawableSize(int& w, int& h) {}

}} // namespace wc::window


#endif
#endif