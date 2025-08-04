#include "bearpch.h"
#include "PipelineLayout.h"
#include "Core/Device.h"
#include "Pipeline/DescriptorSetLayout.h"
namespace Bear {
	PipelineLayout::PipelineLayout(const Device& device, const std::vector<VkDescriptorSetLayout> layouts)
		:m_Device(device)
	{
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
		pipelineLayoutInfo.pSetLayouts = layouts.data();
		BEAR_CORE_ASSERT(vkCreatePipelineLayout(m_Device.GetDevice(), &pipelineLayoutInfo, nullptr, &m_Layout) == VK_SUCCESS, "failed to create pipeline layout!");
	}
	PipelineLayout::~PipelineLayout()
	{
		if (m_Layout != VK_NULL_HANDLE) {
			vkDestroyPipelineLayout(m_Device.GetDevice(), m_Layout, nullptr);
			m_Layout = VK_NULL_HANDLE;
		}
	}
}