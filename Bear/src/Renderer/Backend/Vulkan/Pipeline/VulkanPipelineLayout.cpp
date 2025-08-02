#include "bearpch.h"
#include "VulkanPipelineLayout.h"
#include "Core/VulkanDevice.h"
#include "Pipeline/VulkanDescriptorSetLayout.h"
namespace Bear {
	VulkanPipelineLayout::VulkanPipelineLayout(const VulkanDevice& device, const std::vector<VkDescriptorSetLayout> layouts)
		:m_Device(device)
	{
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
		pipelineLayoutInfo.pSetLayouts = layouts.data();
		BEAR_CORE_ASSERT(vkCreatePipelineLayout(m_Device.GetDevice(), &pipelineLayoutInfo, nullptr, &m_Layout) == VK_SUCCESS, "failed to create pipeline layout!");
	}
	VulkanPipelineLayout::~VulkanPipelineLayout()
	{
		if (m_Layout != VK_NULL_HANDLE) {
			vkDestroyPipelineLayout(m_Device.GetDevice(), m_Layout, nullptr);
			m_Layout = VK_NULL_HANDLE;
		}
	}
}