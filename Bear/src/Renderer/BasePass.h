#pragma once
#include "IRenderPass.h"

namespace Bear
{
	class RHIPipeline;
	class RHIPipelineLayout;
	class RHIRenderPass;
	class RHIFramebuffer;
	class BasePass : public IRenderPass
	{
	public:
		~BasePass() override = default;
		void Setup(RenderContext* context) override;
		void Execute(RHICommandList* cmd, std::vector<RenderObject> renderObjects = {}) override;
		void Resize() override;
		void Cleanup() override;
	private:
		RenderContext* m_Context = nullptr;
		std::shared_ptr<RHIPipeline> m_Pipeline = nullptr;
		std::shared_ptr<RHIPipelineLayout> m_PipelineLayout = nullptr;
		std::shared_ptr<RHIRenderPass> m_RenderPass = nullptr;
		std::vector<std::shared_ptr<RHIFramebuffer>> m_Framebuffers;
	private:
		void Prepare();
		void CreateFramebuffers();
	};
}