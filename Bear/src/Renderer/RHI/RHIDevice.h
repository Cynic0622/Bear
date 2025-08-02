#pragma once
#include <memory>
#include "RHIResources.h"
#include "RHITypes.h"
#include "RHIPipeline.h"
#include "RHIRenderPass.h"
#include "RHISwapchain.h"
namespace Bear {
	class RHIDevice {
		public:
		RHIDevice() = default;
		virtual ~RHIDevice() = default;

		// --- Buffer 工厂方法 ---
		virtual std::unique_ptr<RHIBuffer> CreateBuffer(size_t size, BufferUsage usage, bool cpuAccessible) = 0;
		// --- Descriptor 工厂方法 ---
		virtual std::shared_ptr<RHIDescriptorSetLayout> CreateDescriptorSetLayout(const std::vector<RHIDescriptorSetLayoutBinding>& bindings) = 0;
		// --- PipelineLayout 工厂方法 ---
		virtual std::shared_ptr<RHIPipelineLayout> CreatePipelineLayout(const std::vector<std::shared_ptr<RHIDescriptorSetLayout>>& descriptorSetLayouts) = 0;
		// --- Pipeline 工厂方法 ---
		virtual std::shared_ptr<RHIPipeline> CreatePipeline(const RHIPipelineConfig& config, const RHIRenderPass& renderPass) = 0;
		// --- DescriptorSet 工厂方法 ---
		virtual std::unique_ptr<RHIDescriptorSet> CreateDescriptorSet(std::shared_ptr<RHIDescriptorSetLayout> layout) = 0;
		// --- RenderPass 工厂方法 ---
		virtual std::shared_ptr<RHIRenderPass> CreateRenderPass(const std::vector <RHIAttachmentDescription>& attachments) = 0;
		// --- Swapchain 工厂方法 ---
		virtual std::unique_ptr<RHISwapchain> CreateSwapchain(std::shared_ptr<RHIRenderPass> renderPass) = 0;
		
	};
}