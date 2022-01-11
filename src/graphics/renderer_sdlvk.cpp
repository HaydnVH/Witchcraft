#ifdef PLATFORM_SDL
#ifdef RENDERER_VULKAN

//#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
//#define VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#define vkext VULKAN_HPP_DEFAULT_DISPATCHER


#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.hpp>

#include "renderer.h"

#include "appconfig.h"
#include "userconfig.h"
#include "debug.h"
#include "window.h"

namespace wc {
namespace gfx {

	namespace {

		vk::Instance instance;
		vk::DispatchLoaderDynamic dldi;
		vk::DebugUtilsMessengerEXT debugMessenger;

		vk::PhysicalDevice physicalDevice = nullptr;


		static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT severity,
			VkDebugUtilsMessageTypeFlagsEXT type,
			const VkDebugUtilsMessengerCallbackDataEXT* data,
			void* userdata)
		{
			switch (severity) {
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

		// Application Info.
		vk::ApplicationInfo ai(	appconfig::getAppName().c_str(),
								VK_MAKE_VERSION(appconfig::getMajorVer(), appconfig::getMinorVer(), appconfig::getPatchVer()),
								appconfig::getEngineName().c_str(),
								VK_MAKE_VERSION(appconfig::getEngineMajorVer(), appconfig::getEngineMinorVer(), appconfig::getEnginePatchVer()),
								VK_API_VERSION_1_2);

		// Needed extensions.
		uint32_t count = 0;
		SDL_Vulkan_GetInstanceExtensions(window::getHandle(), &count, nullptr);
		std::vector<const char*> extensions(count);
		SDL_Vulkan_GetInstanceExtensions(window::getHandle(), &count, extensions.data());
#ifndef NDEBUG
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

		// Needed layers.
		std::vector<const char*> layers = {};
#ifndef NDEBUG
		layers.push_back("VK_LAYER_KHRONOS_validation");
#endif

		// Create the vulkan instance.
		{
			vk::InstanceCreateInfo ci({}, &ai, layers, extensions);
#ifndef NDEBUG
			vk::DebugUtilsMessengerCreateInfoEXT dbci({},
				//vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
				//vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
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


		// Enable the debug messenger.
#ifndef NDEBUG
		{
			vk::DebugUtilsMessengerCreateInfoEXT ci({},
				//vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
				//vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
				vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
				vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
				debugCallback, nullptr);
			auto result = instance.createDebugUtilsMessengerEXT(ci, nullptr, dldi);
			if (result.result != vk::Result::eSuccess) {
				debug::fatal("Failed to create Vulkan debug messenger!\n");
				return false;
			}
			debugMessenger = result.value;
		}
#endif

		// Pick a physical decide.
		{
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
			debug::info("Physical devices found:\n");
			for (const auto& device : devices) {
				debug::infomore(device.getProperties().deviceName, '\n');
				//physicalDevice = device;
				//break;
			}

			//if (physicalDevice == VK_NULL_HANDLE) {
			//	debug::fatal("Failed to find a suitable GPU!\n");
			//	return false;
			//}

		}

		return true;
	}

	void shutdown() {
#ifndef NDEBUG
		instance.destroyDebugUtilsMessengerEXT(debugMessenger, nullptr, dldi);
#endif
		vkDestroyInstance(instance, nullptr);
	}

	void drawFrame(float interpolation) {

	}

}} // namespace wc::gfx
#endif // RENDERER_VULKAN
#endif // PLATFORM_SDL