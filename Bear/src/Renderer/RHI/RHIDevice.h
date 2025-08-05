#pragma once
#include <memory>
#include "RHIResources.h"
#include "RHITypes.h"
#include "RHIPipeline.h"
#include "RHIRenderPass.h"
#include "RHISwapchain.h"
#include "RHICommandList.h"
namespace Bear {
	class RHIDevice {
		public:
		RHIDevice() = default;
		virtual ~RHIDevice() = default;

		// --- Buffer factory ---
		virtual std::unique_ptr<RHIBuffer> CreateBuffer(size_t size, BufferUsage usage, bool cpuAccessible) = 0;
		// --- Descriptor factory ---
		virtual std::shared_ptr<RHIDescriptorSetLayout> CreateDescriptorSetLayout(const std::vector<RHIDescriptorSetLayoutBinding>& bindings) = 0;
		// --- PipelineLayout factory ---
		virtual std::shared_ptr<RHIPipelineLayout> CreatePipelineLayout(const std::vector<RHIDescriptorSetLayout*>& descriptorSetLayouts) = 0;
		// --- Pipeline factory ---
		virtual std::shared_ptr<RHIPipeline> CreatePipeline(const RHIPipelineConfig& config, const RHIRenderPass& renderPass) = 0;
		// --- DescriptorSet factory ---
		virtual std::unique_ptr<RHIDescriptorSet> CreateDescriptorSet(std::shared_ptr<RHIDescriptorSetLayout> layout) = 0;
		// --- RenderPass factory ---
		virtual std::shared_ptr<RHIRenderPass> CreateRenderPass(const std::vector <RHIAttachmentDescription>& attachments) = 0;
		// --- Swapchain factory ---
		virtual std::unique_ptr<RHISwapchain> CreateSwapchain(RHIRenderPass& renderPass) = 0;
		// --- Texture factory ---
		virtual std::unique_ptr<RHITexture> CreateTexture(const RHITextureConfig& config) = 0;
		// --- sampler factory ---
		virtual std::shared_ptr<RHISampler> CreateSampler(const RHISamplerConfig& config) = 0;

		// 开始
		virtual RHICommandList& BeginFrame() = 0;
		// 结束并提交
		virtual void EndFrame(RHISwapchain& swapchain, uint32_t imageIndex) = 0;
		// 获取当前 in-flight frame 的索引
		virtual uint32_t GetCurrentFrameIndex() const = 0;

		virtual uint32_t AcquireNextImage(RHISwapchain& swapchain) const = 0;

		virtual void WaitIdle() = 0;
		
	};
}