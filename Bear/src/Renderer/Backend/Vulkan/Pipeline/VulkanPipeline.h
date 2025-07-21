#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <memory>
#include "PipelineConfig.h"
#include "VulkanShader.h"
namespace Bear {

	class VulkanDevice;

	class VulkanPipeline {
		public:
		VulkanPipeline(
			const VulkanDevice& device,
			const std::vector<std::unique_ptr<VulkanShader>>& shaders,
			const PipelineConfigInfo& configInfo);
		~VulkanPipeline();

		void Bind(VkCommandBuffer commandBuffer);
		inline VkPipeline GetHandle() const { return m_GraphicsPipeline; }

		// ½ûÖ¹¿½±´ºÍÒÆ¶¯
		VulkanPipeline(const VulkanPipeline&) = delete;
		VulkanPipeline& operator=(const VulkanPipeline&) = delete;
		VulkanPipeline(VulkanPipeline&&) = delete;
		VulkanPipeline& operator=(VulkanPipeline&&) = delete;

	private:
		const VulkanDevice& m_Device;
		VkPipeline m_GraphicsPipeline = VK_NULL_HANDLE;
	};
}