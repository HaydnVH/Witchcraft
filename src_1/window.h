#ifndef HVH_WC_SYS_WINDOW_H
#define HVH_WC_SYS_WINDOW_H

#ifdef PLATFORM_SDL
struct SDL_Window;
#endif

namespace wc {
namespace window {

	bool init();
	void shutdown();

	bool handleMessages();

	void getWindowSize(int& w, int& h);
	void getDrawableSize(int& w, int& h);

#ifdef PLATFORM_SDL
	SDL_Window* getHandle();
#endif

}} // namespace wc::window

#endif // HVH_WC_SYS_WINDOW_H