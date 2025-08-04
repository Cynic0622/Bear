#include "bearpch.h"

#include "Semaphore.h"
#include "Core/Device.h"

namespace Bear {

	Semaphore::Semaphore(const Device& device, const void* pNext, const VkSemaphoreCreateFlags& flags)
		:m_Device(device)
	{
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreInfo.pNext = pNext;
		semaphoreInfo.flags = flags;
		BEAR_CORE_ASSERT(vkCreateSemaphore(m_Device.GetDevice(), &semaphoreInfo, nullptr, &m_Semaphore) == VK_SUCCESS, "Failed to create Vulkan semaphore!");
	}

	Semaphore::~Semaphore()
	{
		if (m_Semaphore != VK_NULL_HANDLE) {
			vkDestroySemaphore(m_Device.GetDevice(), m_Semaphore, nullptr);
			m_Semaphore = VK_NULL_HANDLE;
		}
	}
}