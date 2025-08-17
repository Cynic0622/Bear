#include "bearpch.h"

#include "CommandPool.h"
#include "Core/Device.h"

namespace Bear {

	CommandPool::CommandPool(const Device& device)
		: m_Device(device)
	{
		QueueFamilyIndices queueFamilyIndices = device.GetQueueFamilyIndices();
		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

		BEAR_CORE_ASSERT(vkCreateCommandPool(device.GetDevice(), &poolInfo, nullptr, &m_CommandPool) == VK_SUCCESS, "Failed to create command pool!");
#ifdef BEAR_DEBUG
		BEAR_CORE_INFO("Vulkan CommandPool created successfully.");
#endif // BEAR_DEBUG
	}
	CommandPool::~CommandPool()
	{
		if (m_CommandPool != VK_NULL_HANDLE) {
			vkDestroyCommandPool(m_Device.GetDevice(), m_CommandPool, nullptr);
			m_CommandPool = VK_NULL_HANDLE;
#ifdef BEAR_DEBUG
			BEAR_CORE_INFO("Vulkan CommandPool destroyed successfully.");
#endif // BEAR_DEBUG
		}
	}
}