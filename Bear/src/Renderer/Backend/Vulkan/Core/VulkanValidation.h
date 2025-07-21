#pragma once
#include <vulkan/vulkan.h>
#include <vector>

namespace Bear {

	class VulkanValidation {

	public:
		explicit VulkanValidation(VkInstance instance);
		~VulkanValidation();

		// 禁止拷贝和移动
		VulkanValidation(const VulkanValidation&) = delete;
		VulkanValidation& operator=(const VulkanValidation&) = delete;
		VulkanValidation(VulkanValidation&&) = delete;
		VulkanValidation& operator=(VulkanValidation&&) = delete;

		static const std::vector<const char*> Layers;
		static bool CheckSupport();

	private:
		static VkResult CreateDebugUtilsMessengerEXT(
			VkInstance instance,
			const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
			const VkAllocationCallbacks* pAllocator,
			VkDebugUtilsMessengerEXT* pDebugMessenger
		);

		static void DestroyDebugUtilsMessengerEXT(
			VkInstance instance,
			VkDebugUtilsMessengerEXT debugMessenger,
			const VkAllocationCallbacks* pAllocator
		);

		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData
		);

	private:
		VkInstance m_Instance; // 保存一份实例句柄，用于析构时销毁 Debug Messenger
		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
	};
}