#pragma once
#include "renderer/RHI/RHIPipeline.h"
#include <vulkan/vulkan.h>>
#include <vector>

namespace Bear {

	class Device;
	class DescriptorSetLayout;
	class PipelineLayout : public RHIPipelineLayout {
	public:
		PipelineLayout(const Device& device, const std::vector<DescriptorSetLayout*>& layouts, const std::vector<VkPushConstantRange>& pushConstantRanges);

		~PipelineLayout() override;

		PipelineLayout(const PipelineLayout&) = delete;
		PipelineLayout& operator=(const PipelineLayout&) = delete;

		VkPipelineLayout GetHandle() const { return m_Layout; }

	private:
		const Device& m_Device;
		VkPipelineLayout m_Layout = VK_NULL_HANDLE;
	};
}