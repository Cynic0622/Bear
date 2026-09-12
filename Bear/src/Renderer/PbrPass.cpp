#include "bearpch.h"
#include "PbrPass.h"
#include "Texture.h"
#include "NtcMaterial.h"
#include "PbrMaterial.h"
#include "Common/Mesh.h"
#include "CullingPass.h"

namespace Bear
{
	PbrPass::~PbrPass()
	{
	}

	void PbrPass::Setup(RenderContext* context)
	{
		m_Context = context;
		CreateRenderPasses();
		CreateFramebuffers();
		CreatePbrDescriptorSetLayout();
		CreatePipeline();
	}

	void PbrPass::Execute(RHICommandList* cmd, const IndirectDrawSet& drawSet, bool clearDepth)
	{
		auto currentImageIndex = m_Context->device->GetCurrentImageIndex();
		auto currentFrameIndex = m_Context->device->GetCurrentFrameIndex();
		auto width = m_Context->swapchain->GetWidth();
		auto height = m_Context->swapchain->GetHeight();

		if (!drawSet.commands || !drawSet.counters || !drawSet.ranges || !drawSet.regionBases)
			return;

		auto& renderPass = clearDepth ? *m_PbrClearRenderPass : *m_PbrLoadRenderPass;
		auto& framebuffer = clearDepth
			? *m_PbrClearFramebuffers[currentImageIndex]
			: *m_PbrLoadFramebuffers[currentImageIndex];

		cmd->BeginRenderPass(renderPass, framebuffer, width, height, { {{}, {}, false}, {{}, {}, true} });
		cmd->SetScissor(0, 0, width, height);
		cmd->SetViewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);

		auto& ranges = *drawSet.ranges;
		const auto& regionBases = *drawSet.regionBases;

		cmd->BindVertexBuffer(*Mesh::GetGlobalVertexBuffer(), 0, 0);
		cmd->BindIndexBuffer(*Mesh::GetGlobalIndexBuffer(), 0);

		for (auto& range : ranges)
		{
			cmd->BindPipeline(*m_PbrPipeline);
			cmd->BindDescriptorSet(*m_PbrPipelineLayout, *m_Context->baseDataDescriptorSet[currentFrameIndex], 0);
			cmd->BindDescriptorSet(*m_PbrPipelineLayout, *m_Context->sceneDataDescriptorSet[currentFrameIndex], 1);
			cmd->BindDescriptorSet(*m_PbrPipelineLayout, *range.material->GetDescriptorSet(), 2);
			cmd->DrawIndexedIndirectCount(*drawSet.commands, *drawSet.counters, range.count,
				sizeof(DrawIndexedIndirectCommand),
				regionBases[range.materialIndex] * sizeof(DrawIndexedIndirectCommand),
				range.materialIndex * sizeof(uint32_t));
		}

		cmd->EndRenderPass();

		m_Stats.opaqueObjects += drawSet.objectCount;
		m_Stats.drawCalls += static_cast<uint32_t>(ranges.size());
	}
	void PbrPass::Resize()
	{
		m_Context->device->WaitIdle();
		Cleanup();
		CreateFramebuffers();
	}

	void PbrPass::Cleanup()
	{
		m_PbrClearFramebuffers.clear();
		m_PbrLoadFramebuffers.clear();
	}

	void PbrPass::CreateRenderPasses()
	{
		// PBR pass that clears depth (first geometry pass of the frame)
		{
			RenderPassDescription desc;
			desc.attachmentCount = 2;
			AttachmentDescription& colorAttachment = desc.attachments[0];
			colorAttachment.format = PixelFormat::B8G8R8A8_SRGB;
			colorAttachment.samples = AttachmentSamples::Count1;
			colorAttachment.loadOp = AttachmentLoadOp::Load;
			colorAttachment.storeOp = AttachmentStoreOp::Store;
			colorAttachment.initialLayout = ImageLayout::ColorAttachment;
			colorAttachment.finalLayout = ImageLayout::ColorAttachment;

			AttachmentDescription& depthAttachment = desc.attachments[1];
			depthAttachment.format = PixelFormat::D32_SFLOAT;
			depthAttachment.samples = AttachmentSamples::Count1;
			depthAttachment.loadOp = AttachmentLoadOp::Clear;
			depthAttachment.storeOp = AttachmentStoreOp::Store;
			depthAttachment.initialLayout = ImageLayout::Undefined;
			depthAttachment.finalLayout = ImageLayout::ShaderReadOnly;

			desc.subpassCount = 1;
			SubpassDescription& subpass = desc.subpasses[0];
			subpass.colorAttachmentCount = 1;
			subpass.colorAttachments[0] = { 0, ImageLayout::ColorAttachment };
			subpass.hasDepthStencil = true;
			subpass.depthStencilAttachment = { 1, ImageLayout::DepthStencilAttachment };

			m_PbrClearRenderPass = m_Context->device->CreateRenderPass(desc);
		}

		// PBR pass that loads existing depth (phase 2 with rescued objects)
		{
			RenderPassDescription desc;
			desc.attachmentCount = 2;
			AttachmentDescription& colorAttachment = desc.attachments[0];
			colorAttachment.format = PixelFormat::B8G8R8A8_SRGB;
			colorAttachment.samples = AttachmentSamples::Count1;
			colorAttachment.loadOp = AttachmentLoadOp::Load;
			colorAttachment.storeOp = AttachmentStoreOp::Store;
			colorAttachment.initialLayout = ImageLayout::ColorAttachment;
			colorAttachment.finalLayout = ImageLayout::ColorAttachment;

			AttachmentDescription& depthAttachment = desc.attachments[1];
			depthAttachment.format = PixelFormat::D32_SFLOAT;
			depthAttachment.samples = AttachmentSamples::Count1;
			depthAttachment.loadOp = AttachmentLoadOp::Load;
			depthAttachment.storeOp = AttachmentStoreOp::Store;
			depthAttachment.initialLayout = ImageLayout::ShaderReadOnly;
			depthAttachment.finalLayout = ImageLayout::ShaderReadOnly;

			desc.subpassCount = 1;
			SubpassDescription& subpass = desc.subpasses[0];
			subpass.colorAttachmentCount = 1;
			subpass.colorAttachments[0] = { 0, ImageLayout::ColorAttachment };
			subpass.hasDepthStencil = true;
			subpass.depthStencilAttachment = { 1, ImageLayout::DepthStencilAttachment };

			m_PbrLoadRenderPass = m_Context->device->CreateRenderPass(desc);
		}
	}

	void PbrPass::CreateFramebuffers()
	{
		uint32_t width = m_Context->swapchain->GetWidth();
		uint32_t height = m_Context->swapchain->GetHeight();
		for (uint32_t i = 0; i < m_Context->swapchain->GetImageCount(); ++i)
		{
			std::vector<void*> attachments = { m_Context->swapchain->GetColorView(i), m_Context->swapchain->GetDepthView(i) };
			m_PbrClearFramebuffers.push_back(m_Context->device->CreateFramebuffer(*m_PbrClearRenderPass, attachments, width, height));
			m_PbrLoadFramebuffers.push_back(m_Context->device->CreateFramebuffer(*m_PbrLoadRenderPass, attachments, width, height));
		}
	}

	void PbrPass::CreatePipeline()
	{
		std::vector<RHIPushConstantRange> pushConstantRanges = {
			{ShaderStage::Vertex, sizeof(PerObjectPushConstants), 0}
		};
		const auto& baseDataDescriptorSetLayout = m_Context->baseDataDescriptorSetLayout;
		const auto& sceneDataDescriptorSetLayout = m_Context->sceneDataDescriptorSetLayout;
		m_PbrPipelineLayout = m_Context->device->CreatePipelineLayout({ baseDataDescriptorSetLayout, sceneDataDescriptorSetLayout, m_DescriptorSetLayout.get()},
			{ pushConstantRanges });

		RHIPipelineConfig pipelineConfig;
		pipelineConfig.pipelineLayout = m_PbrPipelineLayout;
		if (!m_Context->useTextureCompression)
		{
			pipelineConfig.vertexShaderPath = "Bear/src/Shaders/pbrVert.spv";
			pipelineConfig.fragmentShaderPath = "Bear/src/Shaders/pbrFrag.spv";
		}
		else
		{
			pipelineConfig.vertexShaderPath = "Bear/src/Shaders/ntcVS.spv";
			pipelineConfig.fragmentShaderPath = "Bear/src/Shaders/ntcPS.spv";
		}
		pipelineConfig.colorBlendAttachmentState.colorWriteMask = ColorWriteMask::All;
		pipelineConfig.depthStencilState.depthTestEnable = true;
		pipelineConfig.depthStencilState.depthWriteEnable = true;
		pipelineConfig.depthStencilState.depthCompareOp = CompareOp::LessOrEqual;
		pipelineConfig.subpassIndex = 0;
		m_PbrPipeline = m_Context->device->CreatePipeline(pipelineConfig, *m_PbrClearRenderPass);
	}
	void PbrPass::CreatePbrDescriptorSetLayout()
	{
		std::vector<RHIDescriptorSetLayoutBinding> bindings;
		if (!m_Context->useTextureCompression)
		{
			bindings = PbrMaterial::GetDescriptorSetLayoutBinding();
		}
		else
		{
			bindings = NtcMaterial::GetDescriptorSetLayoutBinding();
		}
		m_DescriptorSetLayout = m_Context->device->CreateDescriptorSetLayout(bindings);
	}
}
