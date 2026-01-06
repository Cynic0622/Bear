#include "bearpch.h"
#include "BasePass.h"

namespace Bear
{
	void BasePass::Setup(RenderContext* context)
	{
		m_Context = context;
	}
	void BasePass::Execute(RHICommandList* cmd, std::vector<RenderObject> renderObjects)
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
		for (const auto& obj : renderObjects)
		{
			// bind descriptor sets
			cmd->BindDescriptorSet(*m_PipelineLayout, *m_Context->baseDataDescriptorSet[currentFrameIndex], 0);
			cmd->BindDescriptorSet(*m_PipelineLayout, *m_Context->sceneDataDescriptorSet[currentFrameIndex], 1);
			cmd->BindDescriptorSet(*m_PipelineLayout, *obj.material->GetDescriptorSet(), 2);
			// push constants (model matrix)
			cmd->PushConstants(*m_PipelineLayout, ShaderStage::Vertex, &obj.transform, sizeof(glm::mat4), 0);
			obj.mesh->Draw(*cmd);
		}
	}
	void BasePass::Resize()
	{
	}
	void BasePass::Cleanup()
	{
	}

	void BasePass::Prepare()
	{
		// create render pass
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

		desc.subpassCount = 1;
		SubpassDescription& subpass = desc.subpasses[0];
		subpass.colorAttachmentCount = 1;
		subpass.colorAttachments[0] = { 0, ImageLayout::ColorAttachment };
		subpass.hasDepthStencil = true;
		subpass.depthStencilAttachment = { 1, ImageLayout::DepthStencilAttachment };
		m_RenderPass = m_Context->device->CreateRenderPass(desc);

		// create framebuffers
		CreateFramebuffers();

		// create pipeline layout
		m_PipelineLayout = m_Context->device->CreatePipelineLayout(
			{
				m_Context->baseDataDescriptorSetLayout,
				m_Context->sceneDataDescriptorSetLayout,

			},
			{}
		);
		// create pipeline
		RHIPipelineConfig pipelineConfig;
		pipelineConfig.pipelineLayout = m_PipelineLayout;
		pipelineConfig.vertexShaderPath = "Bear/src/Shaders/baseVert.spv";
		pipelineConfig.fragmentShaderPath = "Bear/src/Shaders/baseFrag.spv";
		pipelineConfig.colorBlendAttachmentState.colorWriteMask = ColorWriteMask::All;
		pipelineConfig.depthStencilState.depthTestEnable = true;
		pipelineConfig.depthStencilState.depthWriteEnable = true;
		pipelineConfig.depthStencilState.depthCompareOp = CompareOp::LessOrEqual;
		pipelineConfig.subpassIndex = 0;
		m_Pipeline = m_Context->device->CreatePipeline(pipelineConfig, *m_RenderPass);
	}

	void BasePass::CreateFramebuffers()
	{
		for (size_t i = 0; i < m_Context->swapchain->GetImageCount(); i++)
		{
			auto framebuffer = m_Context->device->CreateFramebuffer(
				*m_RenderPass, { m_Context->swapchain->GetColorView(i), m_Context->swapchain->GetDepthView() },
				m_Context->swapchain->GetWidth(), m_Context->swapchain->GetHeight());
			m_Framebuffers.push_back(framebuffer);
		}
	}
}
