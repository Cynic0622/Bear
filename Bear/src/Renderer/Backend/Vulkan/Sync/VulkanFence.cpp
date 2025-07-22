#include "bearpch.h"

#include "VulkanFence.h"
#include "Core/VulkanDevice.h"
namespace Bear {

	VulkanFence::VulkanFence(const VulkanDevice& device, bool signaled = false)
		:m_Device(device)
	{
		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = signaled;
		BEAR_CORE_ASSERT(vkCreateFence(device.GetHandle(), &fenceInfo, nullptr, &m_Fence) == VK_SUCCESS, "Failed to create Vulkan fence!");
	}

	VulkanFence::~VulkanFence()
	{
		if (m_Fence != VK_NULL_HANDLE) {
			vkDestroyFence(m_Device.GetHandle(), m_Fence, nullptr);
			m_Fence = VK_NULL_HANDLE;
		}
	}
	void VulkanFence::Wait(uint64_t timeout)
	{
		BEAR_CORE_ASSERT(m_Fence != VK_NULL_HANDLE, "Vulkan fence is not initialized!");
		VkResult result = vkWaitForFences(m_Device.GetHandle(), 1, &m_Fence, VK_TRUE, timeout);
		if (result != VK_SUCCESS && result != VK_TIMEOUT) {
			BEAR_CORE_ERROR("Failed to wait for Vulkan fence: {0}", result);
		}
	}
	void VulkanFence::Reset()
	{
		BEAR_CORE_ASSERT(m_Fence != VK_NULL_HANDLE, "Vulkan fence is not initialized!");
		VkResult result = vkResetFences(m_Device.GetHandle(), 1, &m_Fence);
		if (result != VK_SUCCESS) {
			BEAR_CORE_ERROR("Failed to reset Vulkan fence: {0}", result);
		}
	}
}