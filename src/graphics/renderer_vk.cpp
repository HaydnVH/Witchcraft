#ifdef RENDERER_VULKAN

#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.hpp>

#include "renderer.h"

#include "appconfig.h"
#include "userconfig.h"
#include "debug.h"
#include "window.h"

#include "tools/htable.hpp"

namespace wc {
namespace gfx {

	namespace {

		vk::Instance instance;
		vk::DispatchLoaderDynamic dldi;
		vk::DebugUtilsMessengerEXT debug_messenger;

		vk::SurfaceKHR window_surface;
		vk::PhysicalDevice physical_device;
		vk::Device device;

		vk::Queue graphics_queue;
		vk::Queue present_queue;

		vk::SwapchainKHR swapchain;
		std::vector<vk::Image> swapchain_images;
		std::vector<vk::ImageView> swapchain_views;
		vk::Format swapchain_format;
		vk::Extent2D swapchain_extent;

		struct {

			std::string sPreferredGPU = "";

			bool modified = false;

		} config;


		static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT severity,
			VkDebugUtilsMessageTypeFlagsEXT type,
			const VkDebugUtilsMessengerCallbackDataEXT* data,
			void* userdata)
		{
			switch (severity) {
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: // deliberate overflow
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
				debug::print(debug::INFO, debug::INFOCOLR, "VK INFO: ", debug::CLEAR, data->pMessage, '\n');
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
				debug::print(debug::WARNING, debug::WARNCOLR, "VK WARN: ", debug::CLEAR, data->pMessage, '\n');
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
				debug::print(debug::ERROR, debug::ERRCOLR, "VK ERROR: ", debug::CLEAR, data->pMessage, '\n');
				break;
			default:
				break;
			}

			return VK_FALSE;
		}


	} // namespace <anon>

	bool init() {

		// Load in config settings.
		if (userconfig::exists("renderer")) {
			userconfig::read("renderer", "sPreferredGPU", config.sPreferredGPU);
		}
		else config.modified = true;

		// Initialize the Vulkan instance.
		{
			// Application Info.
			vk::ApplicationInfo ai(	appconfig::getAppName().c_str(),
				VK_MAKE_VERSION(appconfig::getMajorVer(), appconfig::getMinorVer(), appconfig::getPatchVer()),
				appconfig::getEngineName().c_str(),
				VK_MAKE_VERSION(appconfig::getEngineMajorVer(), appconfig::getEngineMinorVer(), appconfig::getEnginePatchVer()),
				VK_API_VERSION_1_2);

			// Neccesary layers.
			std::vector<const char*> layers = {};

			// Neccesary extensions.
		#ifdef PLATFORM_SDL
			uint32_t count = 0;
			SDL_Vulkan_GetInstanceExtensions(window::getHandle(), &count, nullptr);
			std::vector<const char*> extensions(count);
			SDL_Vulkan_GetInstanceExtensions(window::getHandle(), &count, extensions.data());
		#endif
			
		#ifndef NDEBUG
			// Add debugging layers and extensions.
			layers.push_back("VK_LAYER_KHRONOS_validation");
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		#endif


			// Create the instance.
			vk::InstanceCreateInfo ci({}, &ai, layers, extensions);
		#ifndef NDEBUG
			vk::DebugUtilsMessengerCreateInfoEXT dbci({},
				//vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
				//vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
				vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
				vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
				vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
				debugCallback, nullptr);
			ci.setPNext(&dbci);
		#endif
			if (vk::createInstance(&ci, nullptr, &instance) != vk::Result::eSuccess) {
				debug::fatal("Failed to create Vulkan instance!\n");
				return false;
			}
		}
		dldi = vk::DispatchLoaderDynamic(instance, vkGetInstanceProcAddr);


		// Create the debug messenger.
	#ifndef NDEBUG
		{
			vk::DebugUtilsMessengerCreateInfoEXT ci({},
				//vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
				//vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
				vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
				vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
				vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
				debugCallback, nullptr);
			auto result = instance.createDebugUtilsMessengerEXT(ci, nullptr, dldi);
			if (result.result != vk::Result::eSuccess) {
				debug::fatal("Failed to create Vulkan debug messenger!\n");
				return false;
			}
			debug_messenger = result.value;
		}
	#endif

		// Get the window surface.
		{
#ifdef PLATFORM_SDL
			VkSurfaceKHR tmpsurface;
			bool surface_success = SDL_Vulkan_CreateSurface(window::getHandle(), instance, &tmpsurface);
			window_surface = tmpsurface;
			if (!surface_success) {
				debug::fatal("SDL/Vulkan Failed to create window surface!\n");
				debug::errmore(SDL_GetError(), '\n');
				return false;
			}
#endif
		}

		// Pick a physical device.
		uint32_t graphics_family = UINT_MAX;
		uint32_t present_family = UINT_MAX;
		{
			// Get the list of physical devices installed in the system.
			auto result = instance.enumeratePhysicalDevices();
			if (result.result != vk::Result::eSuccess) {
				debug::fatal("Failed to enumerate physical devices!\n");
				return false;
			}
			const auto& pdevices = result.value;
			if (pdevices.size() == 0) {
				debug::fatal("Failed to find any GPUs with Vulkan support!\n");
				return false;
			}
			// Go through the list of physical devices and pick one that's appropriate.
			hvh::htable<std::string, vk::PhysicalDevice, size_t> device_table;
			debug::info("Physical devices found:\n");
			for (const auto& pdevice : pdevices) {
				
				uint32_t found_graphics_family = 0;
				uint32_t found_present_family = 0;

				// Make sure the device supports the queue families we need.
				auto queue_families = pdevice.getQueueFamilyProperties();
				for (uint32_t i = 0; i < (uint32_t)queue_families.size(); ++i) {
					if (queue_families[i].queueFlags & vk::QueueFlagBits::eGraphics) {
						found_graphics_family = i;
					}
					auto result = pdevice.getSurfaceSupportKHR(i, window_surface);
					if (result.result == vk::Result::eSuccess && result.value) {
						found_present_family = i;
					}
				}
				
				// If this GPU doesn't support the queues we need, skip it.
				if (found_graphics_family == UINT_MAX) { continue; }
				if (found_present_family == UINT_MAX) { continue; }

				// Check for neccesary device extensions
				size_t found_extensions = 0;
				hvh::htable<std::string> required_extensions = {
					VK_KHR_SWAPCHAIN_EXTENSION_NAME
				};
				auto result = pdevice.enumerateDeviceExtensionProperties();
				if (result.result != vk::Result::eSuccess) { continue; }
				for (const auto& ext : result.value) {
					if (required_extensions.count(ext.extensionName) > 0) { found_extensions++; }
				}
				if (found_extensions < required_extensions.size()) { continue; }

				// Check for swapchain adequacy
				auto capabilities = pdevice.getSurfaceCapabilitiesKHR(window_surface);
				auto formats = pdevice.getSurfaceFormatsKHR(window_surface);
				auto presentmodes = pdevice.getSurfacePresentModesKHR(window_surface);
				if (capabilities.result != vk::Result::eSuccess ||
					formats.result != vk::Result::eSuccess || formats.value.empty() ||
					presentmodes.result != vk::Result::eSuccess || presentmodes.value.empty())
				{
					continue;
				}

				// Now that we've confirmed that this device is appropriate, save the queue families we got earlier.
				graphics_family = found_graphics_family;
				present_family = found_present_family;

				// Print the name of the device and get its properties.
				debug::infomore(pdevice.getProperties().deviceName);
				vk::PhysicalDeviceMemoryProperties memory = pdevice.getMemoryProperties();

				// Look through the device's memory heaps to find the device-local heap.
				for (const auto& heap : memory.memoryHeaps) {
					if (heap.flags & vk::MemoryHeapFlagBits::eDeviceLocal) {
						// Report how much VRAM this device has.
						debug::print(debug::INFO, " (", heap.size, " bytes of local memory)");
						// Save this device in a table for sorting.
						device_table.insert(std::string(pdevice.getProperties().deviceName.data()), pdevice, heap.size);
						break;
					}
				}
				debug::print(debug::INFO, '\n');
			}
			// Make sure we actually found something.
			if (device_table.size() == 0) {
				debug::fatal("Failed to find any suitable GPU!\n");
				return false;
			}
			// Look for the GPU that the user wanted.
			size_t index = device_table.find(config.sPreferredGPU);
			if (index != SIZE_MAX) {
				physical_device = device_table.at<1>(index);
			}
			// If it can't be found, sort the table so we can use the highest VRAM GPU,
			// then save this GPU into the config.
			else {
				device_table.sort<2>();
				physical_device = device_table.back<1>();
				if (config.sPreferredGPU.size() > 0) {
					debug::warning("In wc::gfx::init() (picking physical device):\n");
					debug::warnmore("Preferred GPU '", config.sPreferredGPU, "' not found.\n");
					debug::warnmore("Using '", physical_device.getProperties().deviceName, "' instead.\n");
				}
				config.sPreferredGPU = std::string(physical_device.getProperties().deviceName.data());
				config.modified = true;
			}
		}

		// Create the logical device
		{
			// The queue create infos we need for the device.
			float queue_priority[] = { 1.0f };
			std::vector<vk::DeviceQueueCreateInfo> qci = {
				{ {}, graphics_family, 1, queue_priority },
				{ {}, present_family, 1, queue_priority }
			};

			// The device extensions we'll need.
			std::vector<const char*> extensions = {
				VK_KHR_SWAPCHAIN_EXTENSION_NAME
			};

			// Features
			vk::PhysicalDeviceFeatures pd_features = {};


			vk::DeviceCreateInfo ci({}, qci, {}, extensions, {});
			auto result = physical_device.createDevice(ci, nullptr, dldi);
			if (result.result != vk::Result::eSuccess) {
				debug::fatal("Vulkan failed to create logical device!\n");
				return false;
			}
			device = result.value;
			graphics_queue = device.getQueue(graphics_family, 0);
			present_queue = device.getQueue(present_family, 0);
		}

		// Swapchain
		{
			// Get some properties of the physical device.
			auto capabilities = physical_device.getSurfaceCapabilitiesKHR(window_surface).value;
			const auto& formats = physical_device.getSurfaceFormatsKHR(window_surface).value;
			const auto& presentmodes = physical_device.getSurfacePresentModesKHR(window_surface).value;

			// Pick a format.  We want 32-bit BGRA SRGB
			vk::SurfaceFormatKHR chosen_format = formats[0];
			for (const auto& format : formats) {
				if (format.format == vk::Format::eB8G8R8A8Srgb &&
					format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
				{
					chosen_format = format;
					break;
				}
			}
			
			// Pick a present mode.  We want mailbox, but will settle for fifo.
			vk::PresentModeKHR chosen_presentmode = vk::PresentModeKHR::eFifo;
			for (const auto& mode : presentmodes) {
				if (mode == vk::PresentModeKHR::eMailbox) {
					chosen_presentmode = mode;
					break;
				}
			}

			// Pick the size of the swapchain.
			vk::Extent2D chosen_swapextent;
			if (capabilities.currentExtent.width != UINT_MAX) {
				chosen_swapextent = capabilities.currentExtent;
			}
			else {
				int width = 0, height = 0;
			#ifdef PLATFORM_SDL
				SDL_Vulkan_GetDrawableSize(window::getHandle(), &width, &height);
			#endif
				chosen_swapextent = vk::Extent2D({
					std::clamp((uint32_t)width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
					std::clamp((uint32_t)height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
				});

			}

			// The number of images we want in our swapchain.
			uint32_t imagecount = capabilities.minImageCount + 1;
			if (capabilities.maxImageCount > 0 && imagecount > capabilities.maxImageCount) {
				imagecount = capabilities.maxImageCount;
			}

			// Create the swapchain.
			vk::SwapchainCreateInfoKHR ci(
				{},
				window_surface,
				imagecount,
				chosen_format.format,
				chosen_format.colorSpace,
				chosen_swapextent,
				1,
				vk::ImageUsageFlagBits::eColorAttachment,
				vk::SharingMode::eExclusive,
				0, nullptr,
				capabilities.currentTransform,
				vk::CompositeAlphaFlagBitsKHR::eOpaque,
				chosen_presentmode,
				true,
				nullptr);

			// Handle queue families being different.
			std::vector<uint32_t> queue_family_indices = { graphics_family, present_family };
			if (graphics_family != present_family) {
				ci.imageSharingMode = vk::SharingMode::eConcurrent;
				ci.setQueueFamilyIndices(queue_family_indices);
			}

			auto result = device.createSwapchainKHR(ci);
			if (result.result != vk::Result::eSuccess) {
				debug::fatal("Vulkan failed to create swapchain!\n");
				return false;
			}
			swapchain = result.value;
			swapchain_images = device.getSwapchainImagesKHR(swapchain).value;
			swapchain_format = chosen_format.format;
			swapchain_extent = chosen_swapextent;
		}

		// Create the image views for our swapchain.
		{
			swapchain_views.resize(swapchain_images.size());
			for (size_t i = 0; i < swapchain_images.size(); ++i) {
				vk::ImageViewCreateInfo ci({},
					swapchain_images[i],
					vk::ImageViewType::e2D,
					swapchain_format,
					vk::ComponentMapping(vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity),
					vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1) );

				auto result = device.createImageView(ci);
				if (result.result != vk::Result::eSuccess) {
					debug::fatal("Vulkan failed to create image view for swapchain image!\n");
					return false;
				}
				swapchain_views[i] = result.value;
			}
		}
		

		return true;
	}

	void shutdown() {
		for (const auto& view : swapchain_views) {
			device.destroyImageView(view);
		}
		device.destroySwapchainKHR(swapchain);
		device.destroy(nullptr, dldi);
		instance.destroySurfaceKHR(window_surface);
	#ifndef NDEBUG
		instance.destroyDebugUtilsMessengerEXT(debug_messenger, nullptr, dldi);
	#endif
		vkDestroyInstance(instance, nullptr);

		if (config.modified) {
			userconfig::write("renderer", "sPreferredGPU", config.sPreferredGPU.c_str());
		}
	}

	void drawFrame(float interpolation) {

	}

}} // namespace wc::gfx
#endif // RENDERER_VULKAN