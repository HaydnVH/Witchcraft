#ifndef HVH_WC_GRAPHICS_RENDERER_H
#define HVH_WC_GRAPHICS_RENDERER_H

namespace wc {
namespace gfx {

	bool init();
	void shutdown();

	void drawFrame(float interpolation);

}} // namespace wc::gfx
#endif // HVH_WC_GRAPHICS_RENDERER_H