#pragma once

#include "IRenderPass.h"

namespace Bear
{
	class SkyboxPass : public Bear::IRenderPass
	{
	public:
		~SkyboxPass() override;
		void Setup(Bear::RenderContext* context) override;
		void Execute(Bear::RHICommandList* cmd, std::vector<Bear::RenderObject> renderObjects = {}) override;
		void Resize() override;
		void Cleanup() override;
		void Prepare();

	private:
		RenderContext* m_Context = nullptr;
		std::shared_ptr<RHIDescriptorSetLayout> m_DescriptorSetLayout;
		std::shared_ptr<RHIPipelineLayout> m_PipelineLayout;
		std::shared_ptr<RHIPipeline> m_Pipeline;
		std::shared_ptr<RHIRenderPass> m_RenderPass;
		std::vector<std::shared_ptr<RHIFramebuffer>> m_Framebuffers;
		std::shared_ptr<RHIDescriptorSet> m_DescriptorSet;
		std::shared_ptr<Texture> m_SkyboxTexture;
	private:
		void CreateFramebuffers();
	};
}