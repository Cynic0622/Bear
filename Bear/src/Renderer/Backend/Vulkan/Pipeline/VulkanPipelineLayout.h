#pragma once
#include "renderer/RHI/RHIPipeline.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace Bear {

	class VulkanDevice;
	class VulkanPipelineLayout : public RHIPipelineLayout {
	public:
		VulkanPipelineLayout(const VulkanDevice& device, const std::vector<VkDescriptorSetLayout> layouts);

		~VulkanPipelineLayout() override;

		VulkanPipelineLayout(const VulkanPipelineLayout&) = delete;
		VulkanPipelineLayout& operator=(const VulkanPipelineLayout&) = delete;

		VkPipelineLayout GetHandle() const { return m_Layout; }

	private:
		const VulkanDevice& m_Device;
		VkPipelineLayout m_Layout = VK_NULL_HANDLE;
	};
}