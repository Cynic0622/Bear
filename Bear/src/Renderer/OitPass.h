#pragma once
#include "Pass.h"
#include "RenderStats.h"
#include "Common/RenderObject.h"

#define MAX_OIT_NODES_PER_PIXEL 4

namespace Bear
{
	class OitPass : public Pass
	{
	public:
		~OitPass() override;

		void Setup(RenderContext* context) override;
		const char* GetName() const override { return "OitPass"; }
		void Execute(RHICommandList* cmd, std::vector<RenderObject> renderObjects);
		void Resize();
		void Cleanup() override;

		const RenderStats& GetStats() const { return m_Stats; }
	private:
		void CreateFramebuffer();
		void CreateRenderPass();
		void CreatePipeline();
		void PrepareOit();
		void PrepareBlend();
	private:
		RenderStats m_Stats;
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
