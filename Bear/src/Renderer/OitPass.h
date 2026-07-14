#pragma once
#include "IRenderPass.h"

#define MAX_OIT_NODES_PER_PIXEL 4

namespace Bear
{
	class OitPass : public IRenderPass
	{
	public:
		~OitPass() override;

		void Setup(RenderContext* context) override;
		void Execute(RHICommandList* cmd, std::vector<RenderObject> renderObjects) override;
		void Resize() override;
		void Cleanup() override;
	private:
		void CreateFramebuffer();
		void CreateRenderPass();
		void CreatePipeline();
		void PrepareOit();
		void PrepareBlend();
	private:
		RenderContext* m_Context = nullptr;
		std::shared_ptr<RHIRenderPass> m_OitRenderPass = nullptr;
		std::shared_ptr<RHIRenderPass> m_BlendRenderPass = nullptr;
		std::vector<std::shared_ptr<RHIFramebuffer>> m_OitFramebuffers;
		std::vector<std::shared_ptr<RHIFramebuffer>> m_BlendFramebuffers;
		std::shared_ptr<RHIPipeline> m_OitPipeline = nullptr;
		std::shared_ptr<RHIPipeline> m_BlendPipeline = nullptr;
		std::shared_ptr<RHIPipelineLayout> m_OitPipelineLayout = nullptr;
		std::shared_ptr<RHIPipelineLayout> m_BlendPipelineLayout = nullptr;
		std::shared_ptr<RHIDescriptorSetLayout> m_OitDescriptorSetLayout = nullptr;
		std::shared_ptr<RHIDescriptorSetLayout> m_PbrDescriptorSetLayout = nullptr;
		std::shared_ptr<RHIDescriptorSetLayout> m_BlendDescriptorSetLayout = nullptr;
		std::shared_ptr<RHIBuffer> m_OitNodeBuffer = nullptr;
		std::shared_ptr<RHIImage> m_PixelCounterImage = nullptr;
		std::shared_ptr<RHISampler> m_PixelCounterSampler = nullptr;
		std::shared_ptr<RHIDescriptorSet> m_OitDescriptorSet = nullptr;
		std::shared_ptr<RHIDescriptorSet> m_BlendDescriptorSet = nullptr;
	};
}
