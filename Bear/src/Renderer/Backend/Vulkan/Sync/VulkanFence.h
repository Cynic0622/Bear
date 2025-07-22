#pragma once
#include <vulkan/vulkan.h>

namespace Bear {

	class VulkanDevice;

	class VulkanFence {

	public:
		VulkanFence(const VulkanDevice& device, bool signaled = false);
		~VulkanFence();

		VulkanFence(const VulkanFence&) = delete;
		VulkanFence& operator=(const VulkanFence&) = delete;

		void Wait(uint64_t timeout = UINT64_MAX);
		void Reset();

		inline VkFence GetHandle() const { return m_Fence; }

	private:
		const VulkanDevice& m_Device;
		VkFence m_Fence = VK_NULL_HANDLE;
	};
}