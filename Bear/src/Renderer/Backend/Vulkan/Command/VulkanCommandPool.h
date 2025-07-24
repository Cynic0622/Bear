#pragma once

#include <vulkan/vulkan.h>

namespace Bear {

	class VulkanDevice;
	class VulkanCommandPool
	{
	public:
		VulkanCommandPool(const VulkanDevice& device);
		~VulkanCommandPool();

		VulkanCommandPool(const VulkanCommandPool&) = delete;
		VulkanCommandPool& operator=(const VulkanCommandPool&) = delete;


		inline VkCommandPool GetHandle() const { return m_CommandPool; }
		inline const VulkanDevice& GetDevice() const { return m_Device; }

	private:
		const VulkanDevice& m_Device;
		VkCommandPool m_CommandPool = VK_NULL_HANDLE;
	};
}