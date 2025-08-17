#include "bearpch.h"
#include "PipelineLayout.h"
#include "Core/Device.h"
#include "Pipeline/DescriptorSetLayout.h"
namespace Bear {
	PipelineLayout::PipelineLayout(const Device& device, const std::vector<DescriptorSetLayout*>& layouts, const std::vector<VkPushConstantRange>& pushConstantRanges)
		:m_Device(device)
	{
		// ½« DescriptorSetLayout ×ª»»Îª VkDescriptorSetLayout
		std::vector<VkDescriptorSetLayout> vkLayouts;
		vkLayouts.reserve(layouts.size());
		for (const auto& layout : layouts) {
			vkLayouts.push_back(layout->GetHandle());
		}
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(vkLayouts.size());
		pipelineLayoutInfo.pSetLayouts = vkLayouts.data();

		pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
		pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();
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