#pragma once

#include <vulkan/vulkan.h>

namespace Bear {

	class Device;
	class DescriptorPool {
	public:
		DescriptorPool(const Device& device, uint32_t maxSets, const std::vector<VkDescriptorPoolSize>& poolSizes);
		~DescriptorPool();

		DescriptorPool(const DescriptorPool&) = delete;
		DescriptorPool& operator=(const DescriptorPool&) = delete;

		VkDescriptorPool GetHandle() const { return m_Pool; }

	private:
		const Device& m_Device;
		VkDescriptorPool m_Pool = VK_NULL_HANDLE;

	};
}