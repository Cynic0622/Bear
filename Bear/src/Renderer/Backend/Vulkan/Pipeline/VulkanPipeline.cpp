#include "bearpch.h"

#include "VulkanPipeline.h"
#include "Core/VulkanUtils.h"
#include "Core/VulkanDevice.h"
#include "Core/VulkanTypes.h"
#include "Pipeline/VulkanPipelineLayout.h"
#include "Pipeline/VulkanRenderPass.h"

namespace Bear {

	VulkanPipeline::VulkanPipeline(const VulkanDevice& device, const VkGraphicsPipelineCreateInfo& pipelineInfo)
		:m_Device(device)
	{
		BEAR_CORE_ASSERT(vkCreateGraphicsPipelines(m_Device.GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline) == VK_SUCCESS, 
			"Failed to create graphics pipeline!");
#ifdef BEAR_DEBUG
		BEAR_CORE_INFO("Vulkan Pipeline created successfully.");
#endif
	}
	VulkanPipeline::~VulkanPipeline()
	{
		if (m_GraphicsPipeline != VK_NULL_HANDLE) {
			vkDestroyPipeline(m_Device.GetDevice(), m_GraphicsPipeline, nullptr);
			m_GraphicsPipeline = VK_NULL_HANDLE;
#ifdef BEAR_DEBUG
			BEAR_CORE_INFO("Vulkan Pipeline destroyed successfully.");
#endif // BEAR_DEBUG

		}
	}
	void VulkanPipeline::Bind(VkCommandBuffer commandBuffer)
	{
		BEAR_CORE_ASSERT(m_GraphicsPipeline != VK_NULL_HANDLE, "Graphics pipeline is null!");
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);
	}
}