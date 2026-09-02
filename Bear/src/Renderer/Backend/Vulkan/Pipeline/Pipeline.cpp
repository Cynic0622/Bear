#include "bearpch.h"

#include "Pipeline.h"
#include "Core/Device.h"

namespace Bear {

	Pipeline::Pipeline(const Device& device, const VkGraphicsPipelineCreateInfo& pipelineInfo)
		:m_Device(device), m_BindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS)
	{
		BEAR_CORE_ASSERT(vkCreateGraphicsPipelines(m_Device.GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) == VK_SUCCESS,
			"Failed to create graphics pipeline!");
#ifdef BEAR_DEBUG
		BEAR_CORE_INFO("Vulkan Pipeline created successfully.");
#endif
	}
	Pipeline::Pipeline(const Device& device, const VkComputePipelineCreateInfo& pipelineInfo)
		:m_Device(device), m_BindPoint(VK_PIPELINE_BIND_POINT_COMPUTE)
	{
		BEAR_CORE_ASSERT(vkCreateComputePipelines(m_Device.GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) == VK_SUCCESS,
			"Failed to create compute pipeline!");
#ifdef BEAR_DEBUG
		BEAR_CORE_INFO("Vulkan Compute Pipeline created successfully.");
#endif
	}
	Pipeline::~Pipeline()
	{
		if (m_Pipeline != VK_NULL_HANDLE) {
			vkDestroyPipeline(m_Device.GetDevice(), m_Pipeline, nullptr);
			m_Pipeline = VK_NULL_HANDLE;
#ifdef BEAR_DEBUG
			BEAR_CORE_INFO("Vulkan Pipeline destroyed successfully.");
#endif // BEAR_DEBUG

		}
	}
	void Pipeline::Bind(VkCommandBuffer commandBuffer)
	{
		BEAR_CORE_ASSERT(m_Pipeline != VK_NULL_HANDLE, "Pipeline is null!");
		vkCmdBindPipeline(commandBuffer, m_BindPoint, m_Pipeline);
	}
}
