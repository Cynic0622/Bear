#pragma once

#include "Pass.h"
#include "Common/RenderObject.h"

namespace Bear
{
	class SkyboxPass : public Bear::Pass
	{
	public:
		~SkyboxPass() override;
		void Setup(Bear::RenderContext* context) override;
		const char* GetName() const override { return "SkyboxPass"; }
		void Execute(Bear::RHICommandList* cmd, std::vector<Bear::RenderObject> renderObjects = {});
		void Resize();
		void Cleanup() override;
		void Prepare();

	private:
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