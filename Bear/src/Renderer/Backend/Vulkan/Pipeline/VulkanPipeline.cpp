#include "bearpch.h"

#include "VulkanPipeline.h"

#include "Core/VulkanDevice.h"

namespace Bear {

	VulkanPipeline::VulkanPipeline(const VulkanDevice& device, const std::vector<std::unique_ptr<VulkanShader>>& shaders, const PipelineConfigInfo& configInfo)
		:m_Device(device)
	{
		BEAR_CORE_ASSERT(configInfo.renderPass != VK_NULL_HANDLE, "Render pass is null in configInfo!");
		BEAR_CORE_ASSERT(configInfo.pipelineLayout != VK_NULL_HANDLE, "Pipeline layout is null in configInfo!");

		std::vector<VkPipelineShaderStageCreateInfo> shaderStageInfos;
		for (const auto& shader : shaders) {
			shaderStageInfos.push_back(shader->GetStageCreateInfo());
		}

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = static_cast<uint32_t>(shaderStageInfos.size());
		pipelineInfo.pStages = shaderStageInfos.data();
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &configInfo.inputAssemblyInfo;
		pipelineInfo.pViewportState = &configInfo.viewportInfo;
		pipelineInfo.pRasterizationState = &configInfo.rasterizationInfo;
		pipelineInfo.pMultisampleState = &configInfo.multisampleInfo;
		pipelineInfo.pColorBlendState = &configInfo.colorBlendInfo;
		pipelineInfo.pDepthStencilState = &configInfo.depthStencilInfo;
		pipelineInfo.pDynamicState = &configInfo.dynamicStateInfo;

		pipelineInfo.layout = configInfo.pipelineLayout;
		pipelineInfo.renderPass = configInfo.renderPass;
		pipelineInfo.subpass = 0; // 我们要使用的子流程索引

		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		BEAR_CORE_ASSERT(vkCreateGraphicsPipelines(m_Device.GetHandle(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline) == VK_SUCCESS, "Failed to create graphics pipeline!");
	}
	VulkanPipeline::~VulkanPipeline()
	{
		if (m_GraphicsPipeline != VK_NULL_HANDLE) {
			vkDestroyPipeline(m_Device.GetHandle(), m_GraphicsPipeline, nullptr);
			m_GraphicsPipeline = VK_NULL_HANDLE;
#ifdef BEAR_DEBUG
			BEAR_CORE_INFO("Vulkan Graphics Pipeline created successfully.");
#endif // BEAR_DEBUG

		}
	}
	void VulkanPipeline::Bind(VkCommandBuffer commandBuffer)
	{
		BEAR_CORE_ASSERT(m_GraphicsPipeline != VK_NULL_HANDLE, "Graphics pipeline is null!");
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);
	}
}