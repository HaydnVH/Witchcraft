#ifndef HVH_WC_SYS_RENDERER_H
#define HVH_WC_SYS_RENDERER_H

namespace sys {
namespace renderer {

	bool Init();
	void Shutdown();

	void DisplayFrame(float ipo);

	void getScreenSize(int& w, int& h);
	void setScreenSize(int w, int h);

}} // namespace sys::renderer

#endif // HVH_WC_SYS_RENDERER_H