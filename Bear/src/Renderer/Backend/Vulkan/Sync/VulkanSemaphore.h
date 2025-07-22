#pragma once
#include <vulkan/vulkan.h>

namespace Bear {

	class VulkanDevice;
	class VulkanSemapgore {
		
	public:
		VulkanSemapgore(const VulkanDevice& device, const void* pNext = nullptr, const VkSemaphoreCreateFlags& flags = 0);
		~VulkanSemapgore();

		VulkanSemapgore(const VulkanSemapgore&) = delete;
		VulkanSemapgore& operator=(const VulkanSemapgore&) = delete;

		inline VkSemaphore GetSemaphore() const { return m_Semaphore; }

	private:
		const VulkanDevice& m_Device;
		VkSemaphore m_Semaphore = VK_NULL_HANDLE;
	};
}