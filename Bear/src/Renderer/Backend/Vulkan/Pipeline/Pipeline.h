#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <memory>
#include "PipelineConfig.h"
#include "Shader.h"
#include "RHI/RHIPipeline.h"
#include "RHI/RHIRenderPass.h"
namespace Bear {

	class Device;
	class RenderPass;
	class Pipeline : public RHIPipeline{
	public:
		/*Pipeline(const Device& device, const std::vector<std::unique_ptr<Shader>>& shaders, const PipelineConfigInfo& configInfo, 
			const VkPipelineBindPoint& bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS);*/
		Pipeline(const Device& device, const VkGraphicsPipelineCreateInfo& pipelineInfo);
		~Pipeline() override;

		void Bind(VkCommandBuffer commandBuffer);
		inline VkPipeline GetHandle() const { return m_GraphicsPipeline; }
		inline VkPipelineBindPoint GetBindPoint() const { return VK_PIPELINE_BIND_POINT_GRAPHICS; }

		// ½ûÖ¹¿½±´ºÍÒÆ¶¯
		Pipeline(const Pipeline&) = delete;
		Pipeline& operator=(const Pipeline&) = delete;
		Pipeline(Pipeline&&) = delete;
		Pipeline& operator=(Pipeline&&) = delete;

	private:
		const Device& m_Device;
		VkPipeline m_GraphicsPipeline = VK_NULL_HANDLE;
	};
}