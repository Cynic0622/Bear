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
		CreatePipeline();
	}

	void OitPass::Execute(RHICommandList* cmd, std::vector<RenderObject> renderObjects, RHIImage* color, RHIImage* depth,
		RHIBuffer* nodeBuffer, RHIImage* pixelCounterImage)
	{
		auto currentFrameIndex = m_Context->device->GetCurrentFrameIndex();
		auto& oitDescriptorSet = m_OitDescriptorSets[currentFrameIndex];
		auto& blendDescriptorSet = m_BlendDescriptorSets[currentFrameIndex];
		oitDescriptorSet->UpdateBuffer(0, *nodeBuffer);
		oitDescriptorSet->UpdateTexture(1, *pixelCounterImage, *m_PixelCounterSampler);
		blendDescriptorSet->UpdateBuffer(0, *nodeBuffer);
		blendDescriptorSet->UpdateTexture(1, *pixelCounterImage, *m_PixelCounterSampler);

		cmd->TransitionImageLayout(*pixelCounterImage, ImageLayout::General, ImageLayout::TransferDst);
		ClearColor clearColor;
		clearColor.uint32[0] = clearColor.uint32[1] = clearColor.uint32[2] = clearColor.uint32[3] = 0;
		cmd->ClearImage(*pixelCounterImage, clearColor);
		cmd->TransitionImageLayout(*pixelCounterImage, ImageLayout::TransferDst, ImageLayout::General);

		MemoryBarrier barrier{};
		barrier.srcAccessMask = AccessFlags::TransferWrite;
		barrier.srcStageMask = PipelineStage::Transfer;
		barrier.dstAccessMask = AccessFlags::ShaderRead | AccessFlags::ShaderWrite;
		barrier.dstStageMask = PipelineStage::FragmentShader;
		cmd->PipelineBarrier(barrier);

		auto width = m_Context->swapchain->GetWidth();
		auto height = m_Context->swapchain->GetHeight();

		// transparent accumulation: depth-tested, depth-write disabled
		std::vector<RHIRenderingAttachment> accumulationAttachments = {
			{ depth, ImageLayout::DepthStencilAttachment, AttachmentLoadOp::Load, AttachmentStoreOp::Store, nullptr, true }
		};
		cmd->BeginRendering(accumulationAttachments);
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
			cmd->BindDescriptorSet(*m_OitPipelineLayout, *oitDescriptorSet, 3);
			PerObjectPushConstants pushConstants{ .model = obj.transform };
			cmd->PushConstants(*m_OitPipelineLayout, ShaderStage::Vertex, &pushConstants, sizeof(PerObjectPushConstants), 0);
			obj.mesh->Draw(*cmd);
		}
		cmd->EndRendering();

		barrier.srcStageMask = PipelineStage::FragmentShader;
		barrier.srcAccessMask = AccessFlags::ShaderWrite;
		barrier.dstStageMask = PipelineStage::FragmentShader;
		barrier.dstAccessMask = AccessFlags::ShaderRead;
		cmd->PipelineBarrier(barrier);

		// blend the accumulated transparency over the opaque color
		std::vector<RHIRenderingAttachment> blendAttachments = {
			{ color, ImageLayout::ColorAttachment, AttachmentLoadOp::Load, AttachmentStoreOp::Store, nullptr, false }
		};
		cmd->BeginRendering(blendAttachments);
		cmd->SetScissor(0, 0, width, height);
		cmd->SetViewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		cmd->BindPipeline(*m_BlendPipeline);
		cmd->BindDescriptorSet(*m_BlendPipelineLayout, *blendDescriptorSet, 0);
		cmd->Draw(3, 1, 0, 0);
		cmd->EndRendering();

		m_Stats.transparentObjects = transparentCount;
		m_Stats.drawCalls = transparentCount + 1; // per-object draws + blend fullscreen triangle
	}

	void OitPass::Cleanup()
	{
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
		pipelineConfig.dynamicRendering = true;
		pipelineConfig.colorFormats = {}; // depth-only rendering
		pipelineConfig.depthFormat = PixelFormat::D32_SFLOAT;
		m_OitPipeline = m_Context->device->CreatePipeline(pipelineConfig);

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
		blendPipelineConfig.dynamicRendering = true;
		blendPipelineConfig.colorFormats = { PixelFormat::B8G8R8A8_SRGB };
		blendPipelineConfig.depthFormat = PixelFormat::Unknown;
		m_BlendPipeline = m_Context->device->CreatePipeline(blendPipelineConfig);
	}
	void OitPass::PrepareOit()
	{
		// the node buffer and pixel counter image are graph-managed transients; only the
		// layouts, sampler and per-frame-in-flight descriptor sets live here
		m_PixelCounterSampler = m_Context->device->CreateSampler(RHISamplerConfig::GetDefault());

		m_OitDescriptorSetLayout = m_Context->device->CreateDescriptorSetLayout({
			{ 0, DescriptorType::StorageBuffer, 1, ShaderStage::Fragment }, // Oit Node Buffer
			{ 1, DescriptorType::StorageImage, 1, ShaderStage::Fragment},  // Pixel Counter Image
			});
		m_PbrDescriptorSetLayout = m_Context->device->CreateDescriptorSetLayout(PbrMaterial::GetDescriptorSetLayoutBinding());

		m_OitDescriptorSets.resize(m_Context->MAX_FRAMES_IN_FLIGHT);
		for (auto& set : m_OitDescriptorSets)
			set = m_Context->device->CreateDescriptorSet(m_OitDescriptorSetLayout);
	}

	void OitPass::PrepareBlend()
	{
		m_BlendDescriptorSetLayout = m_Context->device->CreateDescriptorSetLayout({
			{ 0, DescriptorType::StorageBuffer, 1, ShaderStage::Fragment }, // Oit Node Buffer
			{ 1, DescriptorType::StorageImage, 1, ShaderStage::Fragment},  // Pixel Counter Image (read-only)
			});
		m_BlendDescriptorSets.resize(m_Context->MAX_FRAMES_IN_FLIGHT);
		for (auto& set : m_BlendDescriptorSets)
			set = m_Context->device->CreateDescriptorSet(m_BlendDescriptorSetLayout);
	}
}
