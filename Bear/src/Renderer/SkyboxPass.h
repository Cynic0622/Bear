#pragma once

#include "Pass.h"
#include "Common/RenderObject.h"

namespace Bear
{
	class RHIImage;
	class SkyboxPass : public Bear::Pass
	{
	public:
		~SkyboxPass() override;
		void Setup(Bear::RenderContext* context) override;
		const char* GetName() const override { return "SkyboxPass"; }
		// color/depth are the dynamic-rendering attachments; the frame graph has already
		// transitioned them to ColorAttachment / DepthStencilAttachment.
		void Execute(Bear::RHICommandList* cmd, RHIImage* color, RHIImage* depth);
		void Cleanup() override;
		void Prepare();

	private:
		std::shared_ptr<RHIDescriptorSetLayout> m_DescriptorSetLayout;
		std::shared_ptr<RHIPipelineLayout> m_PipelineLayout;
		std::shared_ptr<RHIPipeline> m_Pipeline;
		std::shared_ptr<RHIDescriptorSet> m_DescriptorSet;
		std::shared_ptr<Texture> m_SkyboxTexture;
	};
}
