#ifndef HVH_WC_SYS_WINDOW_H
#define HVH_WC_SYS_WINDOW_H

namespace window {

	bool Init();
	void Shutdown();

	bool HandleMessages();

	void getWindowSize(int& w, int& h);
	void getDrawableSize(int& w, int& h);

} // namespace window

#endif // HVH_WC_SYS_WINDOW_H