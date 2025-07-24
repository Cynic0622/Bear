#pragma once
#include <vulkan/vulkan.h>

namespace Bear {

	class VulkanDevice;
	class VulkanSemaphore {
		
	public:
		VulkanSemaphore(const VulkanDevice& device, const void* pNext = nullptr, const VkSemaphoreCreateFlags& flags = 0);
		~VulkanSemaphore();

		VulkanSemaphore(const VulkanSemaphore&) = delete;
		VulkanSemaphore& operator=(const VulkanSemaphore&) = delete;

		inline VkSemaphore GetHandle() const { return m_Semaphore; }

	private:
		const VulkanDevice& m_Device;
		VkSemaphore m_Semaphore = VK_NULL_HANDLE;
	};
}