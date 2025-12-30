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
		// set a barrier to make sure last pass(pbr) depth write finished before oit pass read depth.
		MemoryBarrier depthBarrier{};
		depthBarrier.srcAccessMask = AccessFlags::DepthStencilAttachmentWrite;
		depthBarrier.srcStageMask = PipelineStage::LateFragmentTests;
		depthBarrier.dstAccessMask = AccessFlags::DepthStencilAttachmentRead;
		depthBarrier.dstStageMask = PipelineStage::EarlyFragmentTests;
		cmd->PipelineBarrier(depthBarrier);

		// reset buffer value
		uint32_t fillValue = 0;
		cmd->FillBuffer(*m_AtomicCounterBuffer, &fillValue, sizeof(fillValue), offsetof(AtomicCounter, count));
		// reset texture value
		cmd->TransitionImageLayout(*m_HeadPointerImage, ImageLayout::General, ImageLayout::TransferDst);
		ClearColor clearColor;
		clearColor.uint32[0] = clearColor.uint32[1] = clearColor.uint32[2] = clearColor.uint32[3] = 0xFFFFFFFF;
		cmd->ClearImage(*m_HeadPointerImage, clearColor);
		// image layout transferDst --> general
		cmd->TransitionImageLayout(*m_HeadPointerImage, ImageLayout::TransferDst, ImageLayout::General);

		// set barrier, make sure fill and clear finished before fragment shader use.
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
		for (const auto& obj : renderObjects)
		{
			if (!obj.material->IsTransparent())
				continue;
			cmd->BindDescriptorSet(*m_OitPipelineLayout, *m_Context->baseDataDescriptorSet[currentFrameIndex], 0);
			cmd->BindDescriptorSet(*m_OitPipelineLayout, *obj.material->GetDescriptorSet(), 1);
			cmd->BindDescriptorSet(*m_OitPipelineLayout, *m_OitDescriptorSet, 2);
			PerObjectPushConstants pushConstants{ .model = obj.transform };
			cmd->PushConstants(*m_OitPipelineLayout, ShaderStage::Vertex, &pushConstants, sizeof(PerObjectPushConstants), 0);
			obj.mesh->Bind(*cmd);
			obj.mesh->Draw(*cmd);
		}
		cmd->EndRenderPass();
		// set barrier, make sure write(node buffer + head pointer image) finished before next pass use.
		barrier.srcStageMask = PipelineStage::FragmentShader;
		barrier.srcAccessMask = AccessFlags::ShaderWrite;
		barrier.dstStageMask = PipelineStage::FragmentShader;
		barrier.dstAccessMask = AccessFlags::ShaderRead;
		cmd->PipelineBarrier(barrier);

		// Blend Pass
		cmd->BeginRenderPass(*m_BlendRenderPass, *m_BlendFramebuffers[currentImageIndex], width, height, { {{0.1f, 0.1f, 0.1f}, {}, false} });
		cmd->SetScissor(0, 0, width, height);
		cmd->SetViewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		cmd->BindPipeline(*m_BlendPipeline);
		cmd->BindDescriptorSet(*m_BlendPipelineLayout, *m_BlendDescriptorSet, 0);
		cmd->Draw(3, 1, 0, 0);
		cmd->EndRenderPass();
	}

	void OitPass::Resize()
	{
		Cleanup();
		CreateFramebuffer();
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
		auto depthAttachment = m_Context->swapchain->GetDepthView();
		// auto colorAttachment = m_Context->swapchain->GetC();
		for (uint32_t i = 0; i < m_Context->swapchain->GetImageCount(); ++i)
		{
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
		depthAttachment.initialLayout = ImageLayout::DepthStencilAttachment;
		depthAttachment.finalLayout = ImageLayout::DepthStencilAttachment;

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
		m_OitPipelineLayout = m_Context->device->CreatePipelineLayout({ baseDataDescriptorSetLayout, m_PbrDescriptorSetLayout.get() , m_OitDescriptorSetLayout.get() },
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
		blendPipelineConfig.colorBlendAttachmentState.srcColorBlendFactor = BlendFactor::SrcAlpha;
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
		size_t bufferSize = width * height * sizeof(OitNode) * MAX_OIT_NODES_PER_PIXEL;
		m_OitNodeBuffer = m_Context->device->CreateBuffer(bufferSize, BufferUsage::StorageBuffer | BufferUsage::TransferDstBuffer, false);

		AtomicCounter conuter{ .count = 0, .maxCount = width * height * MAX_OIT_NODES_PER_PIXEL };

		auto stagingBuffer = m_Context->device->CreateBuffer(sizeof(AtomicCounter), BufferUsage::StagingBuffer | BufferUsage::TransferSrcBuffer, true);
		stagingBuffer->UploadData(&conuter, sizeof(conuter));
		m_AtomicCounterBuffer = m_Context->device->CreateBuffer(sizeof(AtomicCounter), BufferUsage::StorageBuffer | BufferUsage::TransferDstBuffer, false);
		// m_Context->device->CopyBuffer(stagingBuffer.get(), m_AtomicCounterBuffer.get(), sizeof(AtomicCounter));
		m_Context->device->ImmediateSubmit([&](RHICommandList& cmd)
			{
				cmd.CopyBuffer(*stagingBuffer, *m_AtomicCounterBuffer, sizeof(AtomicCounter), 0, 0);
			});
		stagingBuffer.reset();

		RHITextureConfig textureConfig;
		textureConfig.width = width;
		textureConfig.height = height;
		textureConfig.format = PixelFormat::R32_UINT;
		textureConfig.usage = ImageUsage::Sampled | ImageUsage::Storage | ImageUsage::TransferDst;
		m_HeadPointerImage = m_Context->device->CreateTexture(textureConfig);
		m_HeadPointerSampler = m_Context->device->CreateSampler(RHISamplerConfig::GetDefault());
		// image layout undefined --> general
		m_Context->device->ImmediateSubmit([&](RHICommandList& cmd)
			{
				cmd.TransitionImageLayout(*m_HeadPointerImage, ImageLayout::Undefined, ImageLayout::General);
			});
		// m_RevealageTexture = m_Context->device->CreateTexture(textureConfig);
		m_OitDescriptorSetLayout = m_Context->device->CreateDescriptorSetLayout({
			{ 0, DescriptorType::StorageBuffer, 1, ShaderStage::Fragment }, // Oit Node Buffer
			{ 1, DescriptorType::StorageBuffer, 1, ShaderStage::Fragment}, // Atomic Counter Buffer
			{ 2, DescriptorType::StorageImage, 1, ShaderStage::Fragment}, // Accumulation Texture
			// { 3, DescriptorType::StorageImage, 1, ShaderStage::Fragment}  // Revealage Texture
			});
		m_PbrDescriptorSetLayout = m_Context->device->CreateDescriptorSetLayout(PbrMaterial::GetDescriptorSetLayoutBinding());
		
		m_OitDescriptorSet = m_Context->device->CreateDescriptorSet(m_OitDescriptorSetLayout);
		m_OitDescriptorSet->UpdateBuffer(0, *m_OitNodeBuffer);
		m_OitDescriptorSet->UpdateBuffer(1, *m_AtomicCounterBuffer);
		m_OitDescriptorSet->UpdateTexture(2, *m_HeadPointerImage, *m_HeadPointerSampler);
	}

	void OitPass::PrepareBlend()
	{
		m_BlendDescriptorSetLayout = m_Context->device->CreateDescriptorSetLayout({
			{ 0, DescriptorType::StorageBuffer, 1, ShaderStage::Fragment }, // Oit Node Buffer
			// { 1, DescriptorType::StorageBuffer, 1, ShaderStage::Fragment}, // Atomic Counter Buffer
			{ 1, DescriptorType::StorageImage, 1, ShaderStage::Fragment}, // Accumulation Texture
			// { 3, DescriptorType::StorageImage, 1, ShaderStage::Fragment}  // Revealage Texture
			});
		m_BlendDescriptorSet = m_Context->device->CreateDescriptorSet(m_BlendDescriptorSetLayout);
		m_BlendDescriptorSet->UpdateBuffer(0, *m_OitNodeBuffer);
		m_BlendDescriptorSet->UpdateTexture(1, *m_HeadPointerImage, *m_HeadPointerSampler);
	}
}
