#include "bearpch.h"
#include "SkyboxPass.h"
#include "Texture.h"
namespace Bear
{
	SkyboxPass::~SkyboxPass()
	{
	}
	void SkyboxPass::Setup(RenderContext* context)
	{
		m_Context = context;
		Prepare();
		// load texture and create descriptor set
		std::string filePath = "assets/skybox/qwantani_sunrise_puresky_4k.hdr";
		m_SkyboxTexture = std::make_shared<Texture>(*m_Context->device, filePath);
		m_DescriptorSet->UpdateTexture(0, m_SkyboxTexture->GetImage(), m_SkyboxTexture->GetSampler());
	}

	void SkyboxPass::Execute(RHICommandList* cmd, RHIImage* color, RHIImage* depth)
	{
		auto currentFrameIndex = m_Context->device->GetCurrentFrameIndex();
		auto width = m_Context->swapchain->GetWidth();
		auto height = m_Context->swapchain->GetHeight();

		RHIClearValue colorClear{};
		colorClear.color = { 0.1f, 0.1f, 0.1f, 1.0f };
		RHIClearValue depthClear{};
		depthClear.isDepth = true;
		depthClear.depthStencil.depth = 1.0f;

		std::vector<RHIRenderingAttachment> attachments = {
			{ color, ImageLayout::ColorAttachment, AttachmentLoadOp::Clear, AttachmentStoreOp::Store, &colorClear, false },
			{ depth, ImageLayout::DepthStencilAttachment, AttachmentLoadOp::Clear, AttachmentStoreOp::Store, &depthClear, true }
		};

		cmd->BeginRendering(attachments);
		cmd->SetScissor(0, 0, width, height);
		cmd->SetViewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		cmd->BindPipeline(*m_Pipeline);
		cmd->BindDescriptorSet(*m_PipelineLayout, *m_Context->baseDataDescriptorSet[currentFrameIndex], 0);
		cmd->BindDescriptorSet(*m_PipelineLayout, *m_DescriptorSet, 1);
		// draw a full screen triangle
		cmd->Draw(3, 1, 0, 0);
		cmd->EndRendering();
	}

	void SkyboxPass::Cleanup()
	{
	}

	void SkyboxPass::Prepare()
	{
		m_DescriptorSetLayout = m_Context->device->CreateDescriptorSetLayout(
			{
				{ .binding = 0, .descriptorType = DescriptorType::CombinedImageSampler, .stageFlags = ShaderStage::Fragment }
			}
		);
		m_DescriptorSet = m_Context->device->CreateDescriptorSet(m_DescriptorSetLayout);

		// create pipeline layout
		m_PipelineLayout = m_Context->device->CreatePipelineLayout(
			{ m_Context->baseDataDescriptorSetLayout, m_DescriptorSetLayout.get() },
			{}
		);
		// create pipeline
		RHIPipelineConfig pipelineConfig;
		pipelineConfig.pipelineLayout = m_PipelineLayout;
		pipelineConfig.vertexShaderPath = "Bear/src/Shaders/skyboxVert.spv";
		pipelineConfig.fragmentShaderPath = "Bear/src/Shaders/skyboxFrag.spv";
		pipelineConfig.colorBlendAttachmentState.colorWriteMask = ColorWriteMask::All;
		pipelineConfig.depthStencilState.depthTestEnable = true;
		pipelineConfig.depthStencilState.depthWriteEnable = false;
		pipelineConfig.depthStencilState.depthCompareOp = CompareOp::LessOrEqual;
		pipelineConfig.subpassIndex = 0;
		pipelineConfig.vertexInput = false;
		pipelineConfig.dynamicRendering = true;
		pipelineConfig.colorFormats = { PixelFormat::B8G8R8A8_SRGB };
		pipelineConfig.depthFormat = PixelFormat::D32_SFLOAT;
		m_Pipeline = m_Context->device->CreatePipeline(pipelineConfig);
	}
}
