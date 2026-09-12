#pragma once
#include "Pass.h"
#include "RenderStats.h"

namespace Bear
{
	struct IndirectDrawSet;
	class PbrPass : public Pass
	{
	public:
		~PbrPass() override;

		void Setup(RenderContext* context) override;
		void Cleanup() override;
		const char* GetName() const override { return "PbrPass"; }
		void Resize();

		// Draws a material-bucketed indirect command set into the PBR target.
		// clearDepth = true starts a new depth buffer (first geometry pass of the frame);
		// false loads the existing depth (subsequent passes).
		void Execute(RHICommandList* cmd, const IndirectDrawSet& drawSet, bool clearDepth);

		void ResetStats() { m_Stats = {}; }
		const RenderStats& GetStats() const { return m_Stats; }

	private:
		void CreateRenderPasses();
		void CreateFramebuffers();
		void CreatePipeline();
		void CreatePbrDescriptorSetLayout();

		RenderStats m_Stats;

		// two compatible PBR passes: the first clears depth, the second loads it
		std::shared_ptr<RHIRenderPass> m_PbrClearRenderPass = nullptr;
		std::shared_ptr<RHIRenderPass> m_PbrLoadRenderPass = nullptr;

		std::vector<std::shared_ptr<RHIFramebuffer>> m_PbrClearFramebuffers;
		std::vector<std::shared_ptr<RHIFramebuffer>> m_PbrLoadFramebuffers;

		std::shared_ptr<RHIPipeline> m_PbrPipeline = nullptr;
		std::shared_ptr<RHIPipelineLayout> m_PbrPipelineLayout = nullptr;
		std::shared_ptr<RHIDescriptorSetLayout> m_DescriptorSetLayout = nullptr;
	};
}
