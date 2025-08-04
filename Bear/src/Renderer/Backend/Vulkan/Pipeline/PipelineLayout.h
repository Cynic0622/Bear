#pragma once
#include "renderer/RHI/RHIPipeline.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace Bear {

	class Device;
	class DescriptorSetLayout;
	class PipelineLayout : public RHIPipelineLayout {
	public:
		PipelineLayout(const Device& device, const std::vector<const DescriptorSetLayout*>& layouts);

		~PipelineLayout() override;

		PipelineLayout(const PipelineLayout&) = delete;
		PipelineLayout& operator=(const PipelineLayout&) = delete;

		VkPipelineLayout GetHandle() const { return m_Layout; }

	private:
		const Device& m_Device;
		VkPipelineLayout m_Layout = VK_NULL_HANDLE;
	};
}