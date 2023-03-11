#ifdef RENDERER_VULKAN
#ifndef WC_GFX_RENDERER_VK_H
#define WC_GFX_RENDERER_VK_H

#include "sys/settings.h"

#include <sys/window.h>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace wc::gfx {

  class Renderer {
  public:
    Renderer(wc::SettingsFile& settingsFile, wc::Window& window);
    ~Renderer();

  private:
    wc::SettingsFile& settingsFile_;
    wc::Window&       window_;

    struct Settings {
      std::string sPreferredGPU                     = "";
      bool        bEnableDebug                      = true;
      bool        operator==(const Settings&) const = default;
    };
    Renderer::Settings initialSettings_;
    Renderer::Settings settings_;

    vk::Instance               instance_;
    vk::DispatchLoaderDynamic  dldi_;
    vk::DebugUtilsMessengerEXT debugMessenger_;
    vk::SurfaceKHR             windowSurface_;
    vk::PhysicalDevice         physicalDevice_;
    vk::Device                 device_;
    vk::Queue                  graphicsQueue_;
    vk::Queue                  presentQueue_;

    vk::SwapchainKHR             swapchain_;
    std::vector<vk::Image>       swapchainImages_;
    std::vector<vk::ImageView>   swapchainViews_;
    vk::Format                   swapchainFormat_;
    vk::Extent2D                 swapchainExtent_;
    std::vector<vk::Framebuffer> swapchainFramebuffers_;

    vk::RenderPass     renderPass_;
    vk::PipelineLayout pipelineLayout_;
    vk::Pipeline       graphicsPipeline_;
  };

}  // namespace wc::gfx

#endif  // WC_GFX_RENDERER_VK_H
#endif  // RENDERER_VULKAN