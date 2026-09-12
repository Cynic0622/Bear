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

	void SkyboxPass::Execute(RHICommandList* cmd, std::vector<RenderObject> renderObjects)
	{
		auto currentImageIndex = m_Context->device->GetCurrentImageIndex();
		auto currentFrameIndex = m_Context->device->GetCurrentFrameIndex();
		auto width = m_Context->swapchain->GetWidth();
		auto height = m_Context->swapchain->GetHeight();
		std::vector<RHIClearValue> clearValues = { {{0.1f, 0.1f, 0.1f}, {}, false}, {{}, {}, true} };
		cmd->BeginRenderPass(*m_RenderPass, *m_Framebuffers[currentImageIndex], width, height, clearValues);
		cmd->SetScissor(0, 0, width, height);
		cmd->SetViewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		cmd->BindPipeline(*m_Pipeline);
		cmd->BindDescriptorSet(*m_PipelineLayout, *m_Context->baseDataDescriptorSet[currentFrameIndex], 0);
		cmd->BindDescriptorSet(*m_PipelineLayout, *m_DescriptorSet, 1);
		// draw a full screen triangle
		cmd->Draw(3,1, 0, 0);
		cmd->EndRenderPass();
	}

	void SkyboxPass::Resize()
	{
		m_Context->device->WaitIdle();
		Cleanup();
		CreateFramebuffers();
	}

	void SkyboxPass::Cleanup()
	{
		m_Framebuffers.clear();
	}

	void SkyboxPass::Prepare()
	{
		m_DescriptorSetLayout = m_Context->device->CreateDescriptorSetLayout(
			{
				{ .binding = 0, .descriptorType = DescriptorType::CombinedImageSampler, .stageFlags = ShaderStage::Fragment }
			}
		);
		m_DescriptorSet = m_Context->device->CreateDescriptorSet(m_DescriptorSetLayout);
		// create renderpass
		RenderPassDescription desc;
		desc.attachmentCount = 2; // 1 for depth, 1 for color
		AttachmentDescription& colorAttachment = desc.attachments[0];
		colorAttachment.format = PixelFormat::B8G8R8A8_SRGB;
		colorAttachment.samples = AttachmentSamples::Count1;
		colorAttachment.loadOp = AttachmentLoadOp::Clear;
		colorAttachment.storeOp = AttachmentStoreOp::Store;
		colorAttachment.initialLayout = ImageLayout::Undefined;
		colorAttachment.finalLayout = ImageLayout::ColorAttachment;

		AttachmentDescription& depthAttachment = desc.attachments[1];
		depthAttachment.format = PixelFormat::D32_SFLOAT;
		depthAttachment.samples = AttachmentSamples::Count1;
		depthAttachment.loadOp = AttachmentLoadOp::Clear;
		depthAttachment.storeOp = AttachmentStoreOp::Store;
		depthAttachment.initialLayout = ImageLayout::Undefined;
		depthAttachment.finalLayout = ImageLayout::DepthStencilAttachment;

		desc.subpassCount = 1; // For skybox pass
		SubpassDescription& subpass = desc.subpasses[0];
		subpass.colorAttachmentCount = 1;
		subpass.colorAttachments[0] = { 0, ImageLayout::ColorAttachment };
		subpass.hasDepthStencil = true;
		subpass.depthStencilAttachment = { 1, ImageLayout::DepthStencilAttachment };
		m_RenderPass = m_Context->device->CreateRenderPass(desc);

		// create framebuffer
		CreateFramebuffers();

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
		m_Pipeline = m_Context->device->CreatePipeline(pipelineConfig, *m_RenderPass);
	}

	void SkyboxPass::CreateFramebuffers()
	{
		uint32_t width = m_Context->swapchain->GetWidth();
		uint32_t height = m_Context->swapchain->GetHeight();
		for (uint32_t i = 0; i < m_Context->swapchain->GetImageCount(); ++i)
		{
			m_Framebuffers.push_back(m_Context->device->CreateFramebuffer(*m_RenderPass, { m_Context->swapchain->GetColorView(i), m_Context->swapchain->GetDepthView(i) }, width, height));
		}
	}
}
