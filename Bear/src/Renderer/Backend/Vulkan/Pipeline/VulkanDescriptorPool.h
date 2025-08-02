#pragma once

#include <vulkan/vulkan.h>

namespace Bear {

	class VulkanDevice;
	class VulkanDescriptorPool {
	public:
		VulkanDescriptorPool(const VulkanDevice& device, uint32_t maxSets, const std::vector<VkDescriptorPoolSize>& poolSizes);
		~VulkanDescriptorPool();

		VulkanDescriptorPool(const VulkanDescriptorPool&) = delete;
		VulkanDescriptorPool& operator=(const VulkanDescriptorPool&) = delete;

		VkDescriptorPool GetHandle() const { return m_Pool; }

	private:
		const VulkanDevice& m_Device;
		VkDescriptorPool m_Pool = VK_NULL_HANDLE;

	};
}