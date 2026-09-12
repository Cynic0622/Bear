#include "bearpch.h"
#include "OitPass.h"

#include "PbrMaterial.h"

namespace Bear
{
	OitPass::~OitPass()
	{
	}

	void OitPass::Setup(RenderContext* context)
	{
		m_Context = context;
		PrepareOit();
		PrepareBlend();
		CreateRenderPass();
		CreateFramebuffer();
		CreatePipeline();
	}

	void OitPass::Execute(RHICommandList* cmd, std::vector<RenderObject> renderObjects)
	{
		MemoryBarrier depthBarrier{};
		depthBarrier.srcAccessMask = AccessFlags::DepthStencilAttachmentWrite;
		depthBarrier.srcStageMask = PipelineStage::LateFragmentTests;
		depthBarrier.dstAccessMask = AccessFlags::DepthStencilAttachmentRead;
		depthBarrier.dstStageMask = PipelineStage::EarlyFragmentTests;
		cmd->PipelineBarrier(depthBarrier);

		cmd->TransitionImageLayout(*m_PixelCounterImage, ImageLayout::General, ImageLayout::TransferDst);
		ClearColor clearColor;
		clearColor.uint32[0] = clearColor.uint32[1] = clearColor.uint32[2] = clearColor.uint32[3] = 0;
		cmd->ClearImage(*m_PixelCounterImage, clearColor);
		cmd->TransitionImageLayout(*m_PixelCounterImage, ImageLayout::TransferDst, ImageLayout::General);

		MemoryBarrier barrier{};
		barrier.srcAccessMask = AccessFlags::TransferWrite;
		barrier.srcStageMask = PipelineStage::Transfer;
		barrier.dstAccessMask = AccessFlags::ShaderRead | AccessFlags::ShaderWrite;
		barrier.dstStageMask = PipelineStage::FragmentShader;
		cmd->PipelineBarrier(barrier);

		auto currentImageIndex = m_Context->device->GetCurrentImageIndex();
		auto currentFrameIndex = m_Context->device->GetCurrentFrameIndex();
		auto width = m_Context->swapchain->GetWidth();
		auto height = m_Context->swapchain->GetHeight();

		cmd->BeginRenderPass(*m_OitRenderPass, *m_OitFramebuffers[currentImageIndex], width, height, {});
		cmd->SetScissor(0, 0, width, height);
		cmd->SetViewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		cmd->BindPipeline(*m_OitPipeline);
		uint32_t transparentCount = 0;
		for (const auto& obj : renderObjects)
		{
			if (!obj.material->IsTransparent())
				continue;
			transparentCount++;
			cmd->BindDescriptorSet(*m_OitPipelineLayout, *m_Context->baseDataDescriptorSet[currentFrameIndex], 0);
			cmd->BindDescriptorSet(*m_OitPipelineLayout, *m_Context->sceneDataDescriptorSet[currentFrameIndex], 1);
			cmd->BindDescriptorSet(*m_OitPipelineLayout, *obj.material->GetDescriptorSet(), 2);
			cmd->BindDescriptorSet(*m_OitPipelineLayout, *m_OitDescriptorSet, 3);
			PerObjectPushConstants pushConstants{ .model = obj.transform };
			cmd->PushConstants(*m_OitPipelineLayout, ShaderStage::Vertex, &pushConstants, sizeof(PerObjectPushConstants), 0);
			obj.mesh->Draw(*cmd);
		}
		cmd->EndRenderPass();

		barrier.srcStageMask = PipelineStage::FragmentShader;
		barrier.srcAccessMask = AccessFlags::ShaderWrite;
		barrier.dstStageMask = PipelineStage::FragmentShader;
		barrier.dstAccessMask = AccessFlags::ShaderRead;
		cmd->PipelineBarrier(barrier);

		cmd->BeginRenderPass(*m_BlendRenderPass, *m_BlendFramebuffers[currentImageIndex], width, height, { {{0.1f, 0.1f, 0.1f}, {}, false} });
		cmd->SetScissor(0, 0, width, height);
		cmd->SetViewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		cmd->BindPipeline(*m_BlendPipeline);
		cmd->BindDescriptorSet(*m_BlendPipelineLayout, *m_BlendDescriptorSet, 0);
		cmd->Draw(3, 1, 0, 0);
		cmd->EndRenderPass();

		m_Stats.transparentObjects = transparentCount;
		m_Stats.drawCalls = transparentCount + 1; // per-object draws + blend fullscreen triangle
	}

	void OitPass::Resize()
	{
		Cleanup();
		CreateFramebuffer();
		PrepareOit();
		PrepareBlend();
	}

	void OitPass::Cleanup()
	{
		m_Context->device->WaitIdle();
		m_OitFramebuffers.clear();
		m_BlendFramebuffers.clear();
	}

	void OitPass::CreateFramebuffer()
	{
		uint32_t width = m_Context->swapchain->GetWidth();
		uint32_t height = m_Context->swapchain->GetHeight();
		// auto colorAttachment = m_Context->swapchain->GetC();
		for (uint32_t i = 0; i < m_Context->swapchain->GetImageCount(); ++i)
		{
			auto depthAttachment = m_Context->swapchain->GetDepthView(i);
			m_OitFramebuffers.push_back(m_Context->device->CreateFramebuffer(*m_OitRenderPass, { depthAttachment }, width, height));
			m_BlendFramebuffers.push_back(m_Context->device->CreateFramebuffer(*m_BlendRenderPass, { m_Context->swapchain->GetColorView(i) }, width, height));
		}
	}

	void OitPass::CreateRenderPass()
	{
		RenderPassDescription desc;
		desc.attachmentCount = 1; // 1 for color
		AttachmentDescription& depthAttachment = desc.attachments[0];
		depthAttachment.format = PixelFormat::D32_SFLOAT;
		depthAttachment.samples = AttachmentSamples::Count1;
		depthAttachment.loadOp = AttachmentLoadOp::Load;
		depthAttachment.storeOp = AttachmentStoreOp::DontCare;
		// the depth buffer leaves the PBR passes in ShaderReadOnly (sampled by the Hi-Z build)
		depthAttachment.initialLayout = ImageLayout::ShaderReadOnly;
		depthAttachment.finalLayout = ImageLayout::ShaderReadOnly;

		desc.subpassCount = 1; // For oit pass
		SubpassDescription& subpass = desc.subpasses[0];
		subpass.colorAttachmentCount = 0;
		subpass.hasDepthStencil = true;
		subpass.depthStencilAttachment = { 0, ImageLayout::DepthStencilAttachment };

		desc.dependencyCount = 1;
		SubpassDependency& dependency = desc.dependencies[0];
		dependency.srcSubpass = UINT_MAX;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = PipelineStage::ColorAttachmentOutput | PipelineStage::LateFragmentTests;
		dependency.dstStageMask = PipelineStage::FragmentShader | PipelineStage::LateFragmentTests;
		dependency.srcAccessMask = AccessFlags::DepthStencilAttachmentWrite | AccessFlags::ColorAttachmentWrite;
		dependency.dstAccessMask = AccessFlags::DepthStencilAttachmentRead | AccessFlags::ColorAttachmentRead;

		m_OitRenderPass = m_Context->device->CreateRenderPass(desc);

		RenderPassDescription blendDesc;
		blendDesc.attachmentCount = 1; // 1 for color
		AttachmentDescription& blendColorAttachment = blendDesc.attachments[0];
		blendColorAttachment.format = PixelFormat::B8G8R8A8_SRGB;
		blendColorAttachment.samples = AttachmentSamples::Count1;
		blendColorAttachment.loadOp = AttachmentLoadOp::Load;
		blendColorAttachment.storeOp = AttachmentStoreOp::Store;
		blendColorAttachment.initialLayout = ImageLayout::ColorAttachment;
		blendColorAttachment.finalLayout = ImageLayout::ColorAttachment;

		blendDesc.subpassCount = 1;
		SubpassDescription& blendSubpass = blendDesc.subpasses[0];
		blendSubpass.colorAttachmentCount = 1;
		blendSubpass.colorAttachments[0] = { 0, ImageLayout::ColorAttachment };

		m_BlendRenderPass = m_Context->device->CreateRenderPass(blendDesc);
	}

	void OitPass::CreatePipeline()
	{
		std::vector<RHIPushConstantRange> pushConstantRanges = {
			{ShaderStage::Vertex, sizeof(PerObjectPushConstants), 0}
		};
		const auto& baseDataDescriptorSetLayout = m_Context->baseDataDescriptorSetLayout;
		const auto& sceneDataDescriptorSetLayout = m_Context->sceneDataDescriptorSetLayout;
		m_OitPipelineLayout = m_Context->device->CreatePipelineLayout({ baseDataDescriptorSetLayout, sceneDataDescriptorSetLayout, m_PbrDescriptorSetLayout.get() , m_OitDescriptorSetLayout.get() },
			{ pushConstantRanges });

		RHIPipelineConfig pipelineConfig;
		pipelineConfig.pipelineLayout = m_OitPipelineLayout;
		pipelineConfig.vertexShaderPath = "Bear/src/Shaders/oitVert.spv";
		pipelineConfig.fragmentShaderPath = "Bear/src/Shaders/oitFrag.spv";
		pipelineConfig.colorBlendAttachmentState.colorWriteMask = ColorWriteMask::None;
		pipelineConfig.depthStencilState.depthTestEnable = true;
		pipelineConfig.depthStencilState.depthWriteEnable = false;
		pipelineConfig.depthStencilState.depthCompareOp = CompareOp::Less;
		pipelineConfig.subpassIndex = 0;
		m_OitPipeline = m_Context->device->CreatePipeline(pipelineConfig, *m_OitRenderPass);
		RHIPipelineConfig blendPipelineConfig;
		m_BlendPipelineLayout = m_Context->device->CreatePipelineLayout({ m_BlendDescriptorSetLayout.get() }, {});
		blendPipelineConfig.pipelineLayout = m_BlendPipelineLayout;
		blendPipelineConfig.vertexShaderPath = "Bear/src/Shaders/oitBlendVert.spv";
		blendPipelineConfig.fragmentShaderPath = "Bear/src/Shaders/oitBlendFrag.spv";
		blendPipelineConfig.colorBlendAttachmentState.colorWriteMask = ColorWriteMask::All;
		blendPipelineConfig.colorBlendAttachmentState.blendEnable = true;
		blendPipelineConfig.colorBlendAttachmentState.srcColorBlendFactor = BlendFactor::One;
		blendPipelineConfig.colorBlendAttachmentState.dstColorBlendFactor = BlendFactor::OneMinusSrcAlpha;
		blendPipelineConfig.colorBlendAttachmentState.colorBlendOp = BlendOp::Add;
		blendPipelineConfig.colorBlendAttachmentState.srcAlphaBlendFactor = BlendFactor::One;
		blendPipelineConfig.colorBlendAttachmentState.dstAlphaBlendFactor = BlendFactor::OneMinusSrcAlpha;
		blendPipelineConfig.colorBlendAttachmentState.alphaBlendOp = BlendOp::Add;
		blendPipelineConfig.depthStencilState.depthTestEnable = false;
		blendPipelineConfig.depthStencilState.depthWriteEnable = false;
		blendPipelineConfig.subpassIndex = 0;
		blendPipelineConfig.vertexInput = false;
		m_BlendPipeline = m_Context->device->CreatePipeline(blendPipelineConfig, *m_BlendRenderPass);
	}
	void OitPass::PrepareOit()
	{
		auto width = m_Context->swapchain->GetWidth();
		auto height = m_Context->swapchain->GetHeight();

		// per-pixel continuous node buffer: stride = 32 (vec4 + float in std430)
		size_t nodeStride = 32;
		size_t bufferSize = (size_t)width * height * MAX_OIT_NODES_PER_PIXEL * nodeStride;
		m_OitNodeBuffer = m_Context->device->CreateBuffer(bufferSize, BufferUsage::StorageBuffer | BufferUsage::TransferDstBuffer, false);

		RHITextureConfig textureConfig;
		textureConfig.width = width;
		textureConfig.height = height;
		textureConfig.format = PixelFormat::R32_UINT;
		textureConfig.usage = ImageUsage::Storage | ImageUsage::TransferDst;
		m_PixelCounterImage = m_Context->device->CreateTexture(textureConfig);
		m_PixelCounterSampler = m_Context->device->CreateSampler(RHISamplerConfig::GetDefault());
		m_Context->device->ImmediateSubmit([&](RHICommandList& cmd)
			{
				cmd.TransitionImageLayout(*m_PixelCounterImage, ImageLayout::Undefined, ImageLayout::General);
			});

		m_OitDescriptorSetLayout = m_Context->device->CreateDescriptorSetLayout({
			{ 0, DescriptorType::StorageBuffer, 1, ShaderStage::Fragment }, // Oit Node Buffer
			{ 1, DescriptorType::StorageImage, 1, ShaderStage::Fragment},  // Pixel Counter Image
			});
		m_PbrDescriptorSetLayout = m_Context->device->CreateDescriptorSetLayout(PbrMaterial::GetDescriptorSetLayoutBinding());

		m_OitDescriptorSet = m_Context->device->CreateDescriptorSet(m_OitDescriptorSetLayout);
		m_OitDescriptorSet->UpdateBuffer(0, *m_OitNodeBuffer);
		m_OitDescriptorSet->UpdateTexture(1, *m_PixelCounterImage, *m_PixelCounterSampler);
	}

	void OitPass::PrepareBlend()
	{
		m_BlendDescriptorSetLayout = m_Context->device->CreateDescriptorSetLayout({
			{ 0, DescriptorType::StorageBuffer, 1, ShaderStage::Fragment }, // Oit Node Buffer
			{ 1, DescriptorType::StorageImage, 1, ShaderStage::Fragment},  // Pixel Counter Image (read-only)
			});
		m_BlendDescriptorSet = m_Context->device->CreateDescriptorSet(m_BlendDescriptorSetLayout);
		m_BlendDescriptorSet->UpdateBuffer(0, *m_OitNodeBuffer);
		m_BlendDescriptorSet->UpdateTexture(1, *m_PixelCounterImage, *m_PixelCounterSampler);
	}
}
