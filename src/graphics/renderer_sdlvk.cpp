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

		vk::PhysicalDevice physical_device = nullptr;

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

		// Pick a physical device.
		{
			// Get the list of physical devices installed in the system.
			auto result = instance.enumeratePhysicalDevices();
			if (result.result != vk::Result::eSuccess) {
				debug::fatal("Failed to enumerate physical devices!\n");
				return false;
			}
			auto devices = result.value;
			if (devices.size() == 0) {
				debug::fatal("Failed to find any GPUs with Vulkan support!\n");
				return false;
			}
			// Go through the list of physical devices and pick one that's appropriate.
			hvh::htable<std::string, vk::PhysicalDevice, size_t> device_table;
			debug::info("Physical devices found:\n");
			for (const auto& device : devices) {
				
				// Make sure the device supports the queue families we need.
				auto queue_families = device.getQueueFamilyProperties();
				bool graphics_supported = false;
				for (const auto& family : queue_families) {
					if (family.queueFlags & vk::QueueFlagBits::eGraphics) {
						graphics_supported = true;
					}
				}
				// If this GPU doesn't support what we need, skip it.
				if (!graphics_supported) { continue; }

				// Print the name of the device and get its properties.
				debug::infomore(device.getProperties().deviceName);
				vk::PhysicalDeviceMemoryProperties memory = device.getMemoryProperties();

				// Look through the device's memory heaps to find the device-local heap.
				for (const auto& heap : memory.memoryHeaps) {
					if (heap.flags & vk::MemoryHeapFlagBits::eDeviceLocal) {
						// Report how much VRAM this device has.
						debug::print(debug::INFO, " (", heap.size, " bytes of local memory)");
						// Save this device in a table for sorting.
						device_table.insert(std::string(device.getProperties().deviceName.data()), device, heap.size);
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

		return true;
	}

	void shutdown() {
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