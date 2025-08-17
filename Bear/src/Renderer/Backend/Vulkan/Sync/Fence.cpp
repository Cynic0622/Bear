#include "bearpch.h"

#include "Fence.h"
#include "Core/Device.h"
namespace Bear {

	Fence::Fence(const Device& device, bool signaled)
		:m_Device(device)
	{
		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = signaled;
		BEAR_CORE_ASSERT(vkCreateFence(device.GetDevice(), &fenceInfo, nullptr, &m_Fence) == VK_SUCCESS, "Failed to create Vulkan fence!");
	}

	Fence::~Fence()
	{
		if (m_Fence != VK_NULL_HANDLE) {
			vkDestroyFence(m_Device.GetDevice(), m_Fence, nullptr);
			m_Fence = VK_NULL_HANDLE;
		}
	}
	void Fence::Wait(uint64_t timeout)
	{
		BEAR_CORE_ASSERT(m_Fence != VK_NULL_HANDLE, "Vulkan fence is not initialized!");
		VkResult result = vkWaitForFences(m_Device.GetDevice(), 1, &m_Fence, VK_TRUE, timeout);
		BEAR_CORE_ASSERT(result == VK_SUCCESS || result == VK_TIMEOUT, "Failed to wait for Vulkan fence!");
	}
	void Fence::Reset()
	{
		BEAR_CORE_ASSERT(m_Fence != VK_NULL_HANDLE, "Vulkan fence is not initialized!");
		VkResult result = vkResetFences(m_Device.GetDevice(), 1, &m_Fence);
	}
}