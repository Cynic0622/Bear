#pragma once
#include "Pass.h"
#include "RenderStats.h"
#include "Common/RenderObject.h"

#define MAX_OIT_NODES_PER_PIXEL 4

namespace Bear
{
	class RHIImage;
	class RHIBuffer;
	class OitPass : public Pass
	{
	public:
		~OitPass() override;

		void Setup(RenderContext* context) override;
		const char* GetName() const override { return "OitPass"; }
		// color/depth are the dynamic-rendering attachments (graph-managed transitions);
		// nodeBuffer/pixelCounterImage are graph transients rebuilt every frame
		void Execute(RHICommandList* cmd, std::vector<RenderObject> renderObjects, RHIImage* color, RHIImage* depth,
			RHIBuffer* nodeBuffer, RHIImage* pixelCounterImage);
		void Cleanup() override;

		const RenderStats& GetStats() const { return m_Stats; }
	private:
		void CreatePipeline();
		void PrepareOit();
		void PrepareBlend();
	private:
		RenderStats m_Stats;
		std::shared_ptr<RHIPipeline> m_OitPipeline = nullptr;
		std::shared_ptr<RHIPipeline> m_BlendPipeline = nullptr;
		std::shared_ptr<RHIPipelineLayout> m_OitPipelineLayout = nullptr;
		std::shared_ptr<RHIPipelineLayout> m_BlendPipelineLayout = nullptr;
		std::shared_ptr<RHIDescriptorSetLayout> m_OitDescriptorSetLayout = nullptr;
		std::shared_ptr<RHIDescriptorSetLayout> m_PbrDescriptorSetLayout = nullptr;
		std::shared_ptr<RHIDescriptorSetLayout> m_BlendDescriptorSetLayout = nullptr;
		std::shared_ptr<RHISampler> m_PixelCounterSampler = nullptr;
		std::vector<std::shared_ptr<RHIDescriptorSet>> m_OitDescriptorSets;   // per frame in flight
		std::vector<std::shared_ptr<RHIDescriptorSet>> m_BlendDescriptorSets; // per frame in flight
	};
}
