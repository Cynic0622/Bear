#include "bearpch.h"

#include "Pipeline.h"
#include "Core/Utils.h"
#include "Core/Device.h"
#include "Core/Types.h"
#include "Pipeline/PipelineLayout.h"
#include "Pipeline/RenderPass.h"

namespace Bear {

	Pipeline::Pipeline(const Device& device, const VkGraphicsPipelineCreateInfo& pipelineInfo)
		:m_Device(device)
	{
		BEAR_CORE_ASSERT(vkCreateGraphicsPipelines(m_Device.GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline) == VK_SUCCESS, 
			"Failed to create graphics pipeline!");
#ifdef BEAR_DEBUG
		BEAR_CORE_INFO("Vulkan Pipeline created successfully.");
#endif
	}
	Pipeline::~Pipeline()
	{
		if (m_GraphicsPipeline != VK_NULL_HANDLE) {
			vkDestroyPipeline(m_Device.GetDevice(), m_GraphicsPipeline, nullptr);
			m_GraphicsPipeline = VK_NULL_HANDLE;
#ifdef BEAR_DEBUG
			BEAR_CORE_INFO("Vulkan Pipeline destroyed successfully.");
#endif // BEAR_DEBUG

		}
	}
	void Pipeline::Bind(VkCommandBuffer commandBuffer)
	{
		BEAR_CORE_ASSERT(m_GraphicsPipeline != VK_NULL_HANDLE, "Graphics pipeline is null!");
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);
	}
}