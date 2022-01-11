#ifndef HVH_WC_SYS_WINDOW_H
#define HVH_WC_SYS_WINDOW_H

namespace wc {
namespace window {

	bool init();
	void shutdown();

	bool handleMessages();

	void getWindowSize(int& w, int& h);
	void getDrawableSize(int& w, int& h);

}} // namespace wc::window

#endif // HVH_WC_SYS_WINDOW_H