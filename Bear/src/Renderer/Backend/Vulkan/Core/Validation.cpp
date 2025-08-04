#include "bearpch.h"
#include "Validation.h"
#include "Bear/Log.h"

namespace Bear {

	// 定义静态成员变量
	const std::vector<const char*> Validation::Layers = {
		"VK_LAYER_KHRONOS_validation"
	};
	Validation::Validation(VkInstance instance)
	{
		m_Instance = instance;
		m_DebugMessenger = VK_NULL_HANDLE;

		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = DebugCallback;
		createInfo.pUserData = nullptr; // 可选的用户数据
		BEAR_CORE_ASSERT(CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger) == VK_SUCCESS,
			"Failed to set up debug messenger!");
#ifdef BEAR_DEBUG
		BEAR_CORE_INFO("BEAR::Validation : Vulkan Debug Messenger created.");
#endif // BEAR_DEBUG

	}

	Validation::~Validation()
	{
		if (m_DebugMessenger != VK_NULL_HANDLE) {
			DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
			m_DebugMessenger = VK_NULL_HANDLE;
		}
	}

	bool Validation::CheckSupport()
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		BEAR_CORE_ASSERT(layerCount > 0, "No Vulkan validation layers found!");

		std::vector<VkLayerProperties> availableLayers(layerCount);
		BEAR_CORE_ASSERT(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()) == VK_SUCCESS, "Failed to enumerate Vulkan validation layers!");

		for (const char* layerName : Layers) {
			bool found = false;
			for (const auto& layer : availableLayers) {
				if (strcmp(layerName, layer.layerName) == 0) {
					found = true;
					break;
				}
			}
			if (!found) {
				return false; // 找不到所需的层
			}
		}
		return true;
	}

	VkResult Validation::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		} else {
			return VK_ERROR_EXTENSION_NOT_PRESENT; // 如果函数未找到，返回错误
		}
	}

	void Validation::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(instance, debugMessenger, pAllocator);
		}
	}
	VKAPI_ATTR VkBool32 VKAPI_CALL Validation::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
		VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	{
		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
			BEAR_CORE_WARN("Vulkan Validation Warning: {}", pCallbackData->pMessage);
		} else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
			BEAR_CORE_ERROR("Vulkan Validation Error: {}", pCallbackData->pMessage);
		}

		return VK_FALSE;
	}
}