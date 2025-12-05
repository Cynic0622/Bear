#pragma once
#include "IRenderPass.h"

namespace Bear
{
	struct RenderObject;
	class PbrPass : public IRenderPass
	{
	public:
		~PbrPass() override;

		void Setup(RenderContext* context) override;
		void Execute(RHICommandList* cmd, std::vector<RenderObject> renderObjects = {}) override;
		void Resize() override;
		void Cleanup() override;

	private:
		RenderContext* m_Context = nullptr;
		std::shared_ptr<RHIRenderPass> m_RenderPass = nullptr;
		std::vector<std::shared_ptr<RHIFramebuffer>> m_Framebuffers;
		std::shared_ptr<RHIPipeline> m_PreZPipeline = nullptr;
		std::shared_ptr<RHIPipeline> m_PbrPipeline = nullptr;
		std::shared_ptr<RHIPipelineLayout> m_PreZPipelineLayout = nullptr;
		std::shared_ptr<RHIPipelineLayout> m_PbrPipelineLayout = nullptr;
		std::shared_ptr<RHIDescriptorSetLayout> m_DescriptorSetLayout = nullptr;

	private:
		void CreateRenderPass();
		void CreateFramebuffers();
		void CreatePipeline();
		void CreatePbrDescriptorSetLayout();
	};
}
