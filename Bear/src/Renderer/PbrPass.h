#pragma once
#include "IRenderPass.h"
#include "RenderStats.h"

namespace Bear
{
	struct RenderObject;
	class CullingPass;
	class HiZPass;
	class PbrPass : public IRenderPass
	{
	public:
		~PbrPass() override;

		void Setup(RenderContext* context) override;
		void Execute(RHICommandList* cmd, std::vector<RenderObject> renderObjects = {}) override;
		void Resize() override;
		void Cleanup() override;

		void SetCullingPass(CullingPass* pass) { m_CullingPass = pass; }
		void SetHiZPass(HiZPass* pass) { m_HiZPass = pass; }
		const RenderStats& GetStats() const { return m_Stats; }

	private:
		void CreateRenderPasses();
		void CreateFramebuffers();
		void CreatePipeline();
		void CreatePbrDescriptorSetLayout();
		// draws the material-bucketed command set; rescued selects the phase-2 output (OutB)
		void DrawCommandBuckets(RHICommandList* cmd, bool rescued);

		RenderStats m_Stats;
		CullingPass* m_CullingPass = nullptr;
		HiZPass* m_HiZPass = nullptr;
		RenderContext* m_Context = nullptr;

		// two compatible PBR passes: the first clears depth (start of frame),
		// the second loads it (phase 2 with rescued objects)
		std::shared_ptr<RHIRenderPass> m_PbrClearRenderPass = nullptr;
		std::shared_ptr<RHIRenderPass> m_PbrLoadRenderPass = nullptr;

		std::vector<std::shared_ptr<RHIFramebuffer>> m_PbrClearFramebuffers;
		std::vector<std::shared_ptr<RHIFramebuffer>> m_PbrLoadFramebuffers;

		std::shared_ptr<RHIPipeline> m_PbrPipeline = nullptr;
		std::shared_ptr<RHIPipelineLayout> m_PbrPipelineLayout = nullptr;
		std::shared_ptr<RHIDescriptorSetLayout> m_DescriptorSetLayout = nullptr;

		std::vector<std::unique_ptr<RHIBuffer>> m_IndirectBuffer;
	};
}
