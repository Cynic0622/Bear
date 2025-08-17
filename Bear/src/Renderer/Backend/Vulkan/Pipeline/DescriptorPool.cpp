#include "bearpch.h"

#include "DescriptorPool.h"

#include "Device.h"

namespace Bear {
	DescriptorPool::DescriptorPool(const Device& device, uint32_t maxSets, const std::vector<VkDescriptorPoolSize>& poolSizes)
		: m_Device(device)
	{

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = maxSets;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		BEAR_CORE_ASSERT(vkCreateDescriptorPool(m_Device.GetDevice(), &poolInfo, nullptr, &m_Pool) == VK_SUCCESS, "failed to create descriptor pool!");
	}
	DescriptorPool::~DescriptorPool()
	{
		if (m_Pool != VK_NULL_HANDLE) {
			vkDestroyDescriptorPool(m_Device.GetDevice(), m_Pool, nullptr);
			m_Pool = VK_NULL_HANDLE;
		}
	}
} // namespace Bear