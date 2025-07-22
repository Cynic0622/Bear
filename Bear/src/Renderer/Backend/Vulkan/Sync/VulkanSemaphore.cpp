#include "bearpch.h"

#include "VulkanSemaphore.h"
#include "Core/VulkanDevice.h"

namespace Bear {

	VulkanSemapgore::VulkanSemapgore(const VulkanDevice& device, const void* pNext = nullptr, const VkSemaphoreCreateFlags& flags = 0)
		:m_Device(device)
	{
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreInfo.pNext = pNext;
		semaphoreInfo.flags = flags;
		BEAR_CORE_ASSERT(vkCreateSemaphore(m_Device.GetHandle(), &semaphoreInfo, nullptr, &m_Semaphore) == VK_SUCCESS, "Failed to create Vulkan semaphore!");
	}

	VulkanSemapgore::~VulkanSemapgore()
	{
		if (m_Semaphore != VK_NULL_HANDLE) {
			vkDestroySemaphore(m_Device.GetHandle(), m_Semaphore, nullptr);
			m_Semaphore = VK_NULL_HANDLE;
		}
	}
}