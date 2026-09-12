#pragma once
#include "Pass.h"
#include "RenderStats.h"

namespace Bear
{
	struct IndirectDrawSet;
	class RHIImage;
	class PbrPass : public Pass
	{
	public:
		~PbrPass() override;

		void Setup(RenderContext* context) override;
		void Cleanup() override;
		const char* GetName() const override { return "PbrPass"; }

		// Draws a material-bucketed indirect command set into the PBR target.
		// color/depth are the dynamic-rendering attachments; the frame graph has already
		// transitioned them to ColorAttachment / DepthStencilAttachment.
		// clearDepth = true starts a new depth buffer (first geometry pass of the frame);
		// false loads the existing depth (subsequent passes).
		void Execute(RHICommandList* cmd, const IndirectDrawSet& drawSet, bool clearDepth, RHIImage* color, RHIImage* depth);

		void ResetStats() { m_Stats = {}; }
		const RenderStats& GetStats() const { return m_Stats; }

	private:
		void CreatePipeline();
		void CreatePbrDescriptorSetLayout();

		RenderStats m_Stats;

		std::shared_ptr<RHIPipeline> m_PbrPipeline = nullptr;
		std::shared_ptr<RHIPipelineLayout> m_PbrPipelineLayout = nullptr;
		std::shared_ptr<RHIDescriptorSetLayout> m_DescriptorSetLayout = nullptr;
	};
}
