#ifdef RENDERER_VULKAN
#include "renderer_vk.h"

#include "dbg/debug.h"
#include "sys/appconfig.h"
#include "tools/htable.hpp"

#ifdef PLATFORM_SDL
#include "SDL2/SDL_vulkan.h"
#endif

namespace {
  wc::gfx::Renderer* uniqueRenderer_s = nullptr;

  VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
      VkDebugUtilsMessageTypeFlagsEXT             type,
      const VkDebugUtilsMessengerCallbackDataEXT* data, void* userData) {
    switch (severity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:  // Overflow
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      dbg::info(data->pMessage, "Vulkan");
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      dbg::warning(data->pMessage, "Vulkan");
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      dbg::error(data->pMessage, "Vulkan");
      break;
    default: break;
    }
    return VK_FALSE;
  }

  inline void
      throwOnFail(vk::Result result, std::string_view reason = "",
                  std::source_location src = std::source_location::current()) {
    if (result != vk::Result::eSuccess) {
      throw dbg::Exception((reason == "") ? "Vulkan failure." : reason, src);
    }
  }
}  // namespace

wc::gfx::Renderer::Renderer(wc::SettingsFile& settingsFile, wc::Window& window):
    settingsFile_(settingsFile), window_(window) {

  // Uniqueness check.
  if (uniqueRenderer_s)
    throw dbg::Exception("Only one wc::gfx::Renderer object should exist.");
  uniqueRenderer_s = this;

  // Read settings.
  settingsFile_.read("Renderer.sPreferredGPU", settings_.sPreferredGPU);
  settingsFile_.read("Renderer.bEnableDebug", settings_.bEnableDebug);
  initialSettings_ = settings_;

  // Initialize the Vulkan instance.

  // Application Info.
  vk::ApplicationInfo ai {
      .pApplicationName = wc::APP_NAME,
      .applicationVersion =
          VK_MAKE_VERSION(wc::APP_MAJORVER, wc::APP_MINORVER, wc::APP_PATCHVER),
      .pEngineName   = wc::ENGINE_NAME,
      .engineVersion = VK_MAKE_VERSION(wc::ENGINE_MAJORVER, wc::ENGINE_MINORVER,
                                       wc::ENGINE_PATCHVER),
      .apiVersion    = VK_API_VERSION_1_3};

  // Neccesary layers.
  std::vector<const char*> layers;

  // Neccesary extensions.
#ifdef PLATFORM_SDL
  uint32_t count = 0;
  SDL_Vulkan_GetInstanceExtensions(window_.getHandle(), &count, nullptr);
  std::vector<const char*> extensions(count);
  SDL_Vulkan_GetInstanceExtensions(window_.getHandle(), &count,
                                   extensions.data());
#endif  // PLATFORM_SDL

  // Add debugging layers and extensions.
  if (settings_.bEnableDebug) {
    layers.push_back("VK_LAYER_KHRONOS_validation");
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  // Create the instance.
  vk::InstanceCreateInfo instanceCreateInfo {
      .pApplicationInfo        = &ai,
      .enabledLayerCount       = (uint32_t)layers.size(),
      .ppEnabledLayerNames     = layers.data(),
      .enabledExtensionCount   = (uint32_t)extensions.size(),
      .ppEnabledExtensionNames = extensions.data()};

  vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo {};
  if (settings_.bEnableDebug) {
    debugCreateInfo = {
        .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                           vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                       vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                       vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
        .pfnUserCallback = debugCallback};
    instanceCreateInfo.setPNext(&debugCreateInfo);
  }
  auto [result, instance] = vk::createInstance(instanceCreateInfo);
  throwOnFail(result, "Failed to create Vulkan instance.");
  instance_ = instance;

  dldi_ = vk::DispatchLoaderDynamic(instance_, vkGetInstanceProcAddr);

  // Create the debug messenger.
  if (settings_.bEnableDebug) {
    auto [result, debug] =
        instance_.createDebugUtilsMessengerEXT(debugCreateInfo, nullptr, dldi_);
    throwOnFail(result, "Failed to create Vulkan debug messenger.");
    debugMessenger_ = debug;
  }

  // Create the window surface.
  {
#ifdef PLATFORM_SDL
    VkSurfaceKHR surface;
    bool         result =
        SDL_Vulkan_CreateSurface(window_.getHandle(), instance_, &surface);
    windowSurface_ = surface;
    if (!result)
      throw dbg::Exception(std::format(
          "Failed to create SDL window surface.\n{}", SDL_GetError()));
#endif  // PLATFORM_SDL
  }

  // Pick a physical device.
  uint32_t graphicsFamily = UINT_MAX;
  uint32_t presentFamily  = UINT_MAX;
  {
    // Get the list of physical devices installed in the system.
    auto [result, pdevices] = instance.enumeratePhysicalDevices();
    throwOnFail(result, "Failed to enumerate physical GPU devices.");
    if (pdevices.size() == 0)
      throw dbg::Exception("Failed to find any GPUs with Vulkan support.");

    // Go through the list of physical devices and pick one that's appropriate.
    hvh::Table<std::string, vk::PhysicalDevice, size_t> deviceTable;
    dbg::info("Physical GPUs found:");
    for (const auto& pdevice : pdevices) {
      uint32_t foundGraphicsFamily = UINT_MAX;
      uint32_t foundPresentFamily  = UINT_MAX;

      // Make sure the device supports the queue families we need.
      auto queueFamilies = pdevice.getQueueFamilyProperties();
      for (uint32_t i = 0; i < (uint32_t)queueFamilies.size(); ++i) {
        if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) {
          foundGraphicsFamily = i;
        }
        auto [result, value] = pdevice.getSurfaceSupportKHR(i, windowSurface_);
        if (result == vk::Result::eSuccess && value) {
          foundPresentFamily = i;
        }
      }
      // If this GPU doesn't support the queues we need, skip it.
      if (foundGraphicsFamily == UINT_MAX || foundPresentFamily == UINT_MAX)
        continue;

      // Check for neccesary device extensions
      size_t                  foundExtensions    = 0;
      hvh::Table<std::string> requiredExtensions = {
          VK_KHR_SWAPCHAIN_EXTENSION_NAME};

      auto [result, extensions] = pdevice.enumerateDeviceExtensionProperties();
      if (result != vk::Result::eSuccess)
        continue;

      for (const auto& ext : extensions) {
        if (requiredExtensions.count(ext.extensionName) > 0)
          foundExtensions++;
      }
      if (foundExtensions < requiredExtensions.size())
        continue;

      // Check for swapchain adequacy
      auto capabilities = pdevice.getSurfaceCapabilitiesKHR(windowSurface_);
      auto formats      = pdevice.getSurfaceFormatsKHR(windowSurface_);
      auto presentModes = pdevice.getSurfacePresentModesKHR(windowSurface_);
      if (capabilities.result != vk::Result::eSuccess ||
          formats.result != vk::Result::eSuccess || formats.value.empty() ||
          presentModes.result != vk::Result::eSuccess ||
          presentModes.value.empty())
        continue;

      // Now that we've confirmed that this device is appropriate, save the
      // queue families we got earlier.
      graphicsFamily = foundGraphicsFamily;
      presentFamily  = foundPresentFamily;

      // Print the name of the device and gets its properties.
      std::string deviceReport = pdevice.getProperties().deviceName;
      auto        memory       = pdevice.getMemoryProperties();

      // Look through the device's memory heaps to find the device-local heap.
      for (const auto& heap : memory.memoryHeaps) {
        if (heap.flags & vk::MemoryHeapFlagBits::eDeviceLocal) {
          // Report how much VRAM this device has.
          deviceReport += std::format(" ({} bytes of local memory)", heap.size);
          // Save this device in a table for sorting.
          deviceTable.insert(pdevice.getProperties().deviceName.data(), pdevice,
                             heap.size);
          break;
        }
      }
      dbg::infomore(deviceReport);
    }
    // Make sure we actually found something.
    if (deviceTable.size() == 0)
      throw dbg::Exception("Failed to find any suitable GPU.");

    // Look for the GPU that the user wanted.
    size_t index = deviceTable.find(settings_.sPreferredGPU)->index();
    if (index != SIZE_MAX) {
      physicalDevice_ = deviceTable.at<1>(index);
    } else {
      // If the preferred GPU couldn't be found, sort the table so we can use
      // the highest VRAM GPU, then save this GPU into the config.
      deviceTable.sort<2>();
      physicalDevice_ = deviceTable.back<1>();
      if (settings_.sPreferredGPU.size() > 0) {
        dbg::warning(
            std::format("Preferred GPU '{}' not found.\n"
                        "Using '{}' instead.",
                        settings_.sPreferredGPU,
                        (std::string_view)physicalDevice_.getProperties().deviceName));
      }
      settings_.sPreferredGPU =
          physicalDevice_.getProperties().deviceName.data();
    }
  }

  // Create the logical device.
  {
    // The queue create infos we need for the device.
    float queuePriority[] = {1.0f};

    std::vector<vk::DeviceQueueCreateInfo> qci = {
        {.queueFamilyIndex = graphicsFamily,
         .queueCount       = 1,
         .pQueuePriorities = queuePriority},
        {.queueFamilyIndex = presentFamily,
         .queueCount       = 1,
         .pQueuePriorities = queuePriority}};

    // The device extensions we'll need.
    std::vector<const char*> extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    // Features
    vk::PhysicalDeviceFeatures pdFeatures {};

    vk::DeviceCreateInfo deviceCreateInfo {
        .queueCreateInfoCount    = (uint32_t)qci.size(),
        .pQueueCreateInfos       = qci.data(),
        .enabledExtensionCount   = (uint32_t)extensions.size(),
        .ppEnabledExtensionNames = extensions.data()};

    auto [result, device] = physicalDevice_.createDevice(deviceCreateInfo);
    throwOnFail(result, "Failed to create logical Vulkan device.");

    device_        = device;
    graphicsQueue_ = device_.getQueue(graphicsFamily, 0);
    presentQueue_  = device_.getQueue(presentFamily, 0);
  }

  // Swapchain
  {
    // Get some properties of the physical device
    auto&& [result1, capabilities] =
        physicalDevice_.getSurfaceCapabilitiesKHR(windowSurface_);
    throwOnFail(result1);
    auto&& [result2, formats] =
        physicalDevice_.getSurfaceFormatsKHR(windowSurface_);
    throwOnFail(result2);
    auto&& [result3, presentModes] =
        physicalDevice_.getSurfacePresentModesKHR(windowSurface_);
    throwOnFail(result3);

    // Pick a format.  We want 32-bit BGRA SRGB.
    vk::SurfaceFormatKHR chosenFormat = formats[0];
    for (const auto& format : formats) {
      if (format.format == vk::Format::eB8G8R8A8Srgb &&
          format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
        chosenFormat = format;
        break;
      }
    }

    // Pick a present mode.  We want mailbox, but will settle for fifo.
    vk::PresentModeKHR chosenPresentMode = vk::PresentModeKHR::eFifo;
    for (const auto& mode : presentModes) {
      if (mode == vk::PresentModeKHR::eMailbox) {
        chosenPresentMode = mode;
        break;
      }
    }

    // Pick the size of the swapchain.
    vk::Extent2D chosenSwapExtent;
    if (capabilities.currentExtent.width != UINT_MAX) {
      chosenSwapExtent = capabilities.currentExtent;
    } else {
      int width = 0, height = 0;
      window_.getDrawableSize(width, height);
      chosenSwapExtent = vk::Extent2D(
          {std::clamp((uint32_t)width, capabilities.minImageExtent.width,
                      capabilities.maxImageExtent.width),
           std::clamp((uint32_t)height, capabilities.minImageExtent.height,
                      capabilities.maxImageExtent.height)});
    }

    // The number of images we want in our swapchain.
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 &&
        imageCount > capabilities.maxImageCount) {
      imageCount = capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR swapchainInfo {
        .surface          = windowSurface_,
        .minImageCount    = imageCount,
        .imageFormat      = chosenFormat.format,
        .imageColorSpace  = chosenFormat.colorSpace,
        .imageExtent      = chosenSwapExtent,
        .imageArrayLayers = 1,
        .imageUsage       = vk::ImageUsageFlagBits::eColorAttachment,
        .preTransform     = capabilities.currentTransform,
        .presentMode      = chosenPresentMode,
        .clipped          = true};

    // Handle queue families being different.
    std::vector<uint32_t> queueFamilyIndices = {graphicsFamily, presentFamily};
    if (graphicsFamily != presentFamily) {
      swapchainInfo.imageSharingMode = vk::SharingMode::eConcurrent;
      swapchainInfo.setQueueFamilyIndices(queueFamilyIndices);
    }

    auto [result4, swapchain] = device_.createSwapchainKHR(swapchainInfo);
    throwOnFail(result4, "Failed to create swapchain.");
    swapchain_             = swapchain;
    auto [result5, images] = device_.getSwapchainImagesKHR(swapchain);
    throwOnFail(result5, "Failed to get swapchain images.");
    swapchainImages_ = images;
    swapchainFormat_ = chosenFormat.format;
    swapchainExtent_ = chosenSwapExtent;
  }

  // Create the image views for our swapchain.
  {
    swapchainViews_.reserve(swapchainImages_.size());
    for (const auto& img : swapchainImages_) {
      vk::ImageViewCreateInfo ci {
          .image      = img,
          .viewType   = vk::ImageViewType::e2D,
          .format     = swapchainFormat_,
          .components = vk::ComponentMapping(
              vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity,
              vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity),
          .subresourceRange = vk::ImageSubresourceRange(
              vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)};

      auto [result, view] = device_.createImageView(ci);
      throwOnFail(result, "Failed to create image view for swapchain image.");
      swapchainViews_.push_back(view);
    }
  }

  // Create the render pass.
  {}
}

wc::gfx::Renderer::~Renderer() {
  if (device_) {
    if (graphicsPipeline_)
      device_.destroyPipeline(graphicsPipeline_);
    if (pipelineLayout_)
      device_.destroyPipelineLayout(pipelineLayout_);
    if (renderPass_)
      device_.destroyRenderPass(renderPass_);

    for (const auto& view : swapchainViews_) {
      device_.destroyImageView(view);
    }
    if (swapchain_)
      device_.destroySwapchainKHR(swapchain_);
    device_.destroy();
  }
  if (instance_) {
    if (windowSurface_)
      instance_.destroySurfaceKHR(windowSurface_);
    if (debugMessenger_)
      instance_.destroyDebugUtilsMessengerEXT(debugMessenger_, nullptr, dldi_);
    instance_.destroy();
  }

  if (settings_ != initialSettings_) {
    settingsFile_.write("Renderer.sPreferredGPU", settings_.sPreferredGPU);
    settingsFile_.write("Renderer.bEnableDebug", settings_.bEnableDebug);
  }
}

#endif  // RENDERER_VULKAN