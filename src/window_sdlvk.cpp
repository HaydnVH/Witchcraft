#ifdef PLATFORM_SDL
#ifdef RENDERER_VULKAN
#include <SDL.h>
#include <SDL_vulkan.h>

#include "appconfig.h"
#include "debug.h"

namespace {

	SDL_Window* _window = nullptr;

} // namespace <anon>

namespace wc {
namespace window {

	bool Init() {
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

	void Shutdown() {
		if (_window) {
			SDL_DestroyWindow(_window);
		}
	}

	bool HandleMessages() {
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