#include "bearpch.h"
#include "PbrPass.h"
#include "Texture.h"
#include "NtcMaterial.h"
#include "PbrMaterial.h"
#include "Common/Mesh.h"
#include "CullingPass.h"
#include "HiZPass.h"

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

		m_IndirectBuffer.resize(context->MAX_FRAMES_IN_FLIGHT);
		for (auto& buf : m_IndirectBuffer)
		{
			buf = context->device->CreateBuffer(
				4096 * sizeof(DrawIndexedIndirectCommand),
				BufferUsage::IndirectBuffer | BufferUsage::TransferDstBuffer, true);
		}
	}

	void PbrPass::DrawCommandBuckets(RHICommandList* cmd, bool rescued)
	{
		auto& ranges = m_CullingPass->GetMaterialRanges();
		const auto& regionBases = m_CullingPass->GetRegionBases();
		auto frameIndex = m_Context->device->GetCurrentFrameIndex();

		RHIBuffer* commands = rescued ? m_CullingPass->GetRescuedCommandsBuffer() : m_CullingPass->GetOutputCommandsBuffer();
		RHIBuffer* counters = rescued ? m_CullingPass->GetRescuedCountersBuffer() : m_CullingPass->GetCountersBuffer();

		cmd->BindVertexBuffer(*Mesh::GetGlobalVertexBuffer(), 0, 0);
		cmd->BindIndexBuffer(*Mesh::GetGlobalIndexBuffer(), 0);

		for (auto& range : ranges)
		{
			cmd->BindPipeline(*m_PbrPipeline);
			cmd->BindDescriptorSet(*m_PbrPipelineLayout, *m_Context->baseDataDescriptorSet[frameIndex], 0);
			cmd->BindDescriptorSet(*m_PbrPipelineLayout, *m_Context->sceneDataDescriptorSet[frameIndex], 1);
			cmd->BindDescriptorSet(*m_PbrPipelineLayout, *range.material->GetDescriptorSet(), 2);
			cmd->DrawIndexedIndirectCount(*commands, *counters, range.count,
				sizeof(DrawIndexedIndirectCommand),
				regionBases[range.materialIndex] * sizeof(DrawIndexedIndirectCommand),
				range.materialIndex * sizeof(uint32_t));
		}
	}

	void PbrPass::Execute(RHICommandList* cmd, std::vector<RenderObject> renderObjects)
	{
		auto currentImageIndex = m_Context->device->GetCurrentImageIndex();
		auto currentFrameIndex = m_Context->device->GetCurrentFrameIndex();
		auto width = m_Context->swapchain->GetWidth();
		auto height = m_Context->swapchain->GetHeight();

		const bool gpuCulling = m_CullingPass && m_CullingPass->IsEnabled() && m_CullingPass->GetObjectCount() > 0;
		const bool occlusion = gpuCulling && m_CullingPass->IsOcclusionEnabled() && m_HiZPass != nullptr;

		if (gpuCulling)
		{
			// ---------- PBR1: early-visible set (A); clears depth ----------
			cmd->BeginRenderPass(*m_PbrClearRenderPass, *m_PbrClearFramebuffers[currentImageIndex], width, height, { {{}, {}, false}, {{}, {}, true} });
			cmd->SetScissor(0, 0, width, height);
			cmd->SetViewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
			DrawCommandBuckets(cmd, false);
			cmd->EndRenderPass();

			uint32_t drawCalls = static_cast<uint32_t>(m_CullingPass->GetMaterialRanges().size());

			if (occlusion)
			{
				// Hi-Z from PBR1 depth, then retest rejected objects against it
				m_HiZPass->Execute(cmd, *m_Context->swapchain->GetDepthImage(currentImageIndex), currentFrameIndex);
				m_CullingPass->SetHiZSource(m_HiZPass->GetPyramid(currentFrameIndex), m_HiZPass->GetSampler());
				m_CullingPass->ExecutePhase2(cmd);

				// ---------- PBR2: rescued set (B); loads depth ----------
				cmd->BeginRenderPass(*m_PbrLoadRenderPass, *m_PbrLoadFramebuffers[currentImageIndex], width, height, { {{}, {}, false}, {{}, {}, true} });
				cmd->SetScissor(0, 0, width, height);
				cmd->SetViewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
				DrawCommandBuckets(cmd, true);
				cmd->EndRenderPass();

				// N.B. the second Hi-Z build (complete depth for the next frame) is skipped:
				// the next frame's phase 1 then tests against the phase-1 depth, which is
				// conservative (missing rescued objects as occluders) but halves the Hi-Z cost.
				drawCalls += static_cast<uint32_t>(m_CullingPass->GetMaterialRanges().size());
			}

			m_Stats.opaqueObjects = m_CullingPass->GetObjectCount();
			m_Stats.drawCalls = drawCalls;
			const auto& gpu = m_CullingPass->GetGpuStats();
			m_Stats.gpuVisible = gpu.visibleA;
			m_Stats.gpuRejected = gpu.rejected;
			m_Stats.gpuRescued = gpu.rescued;
			return;
		}

		// ---------------- CPU fallback path (no GPU culling) ----------------
		std::vector<size_t> opaqueIdx;
		for (size_t i = 0; i < renderObjects.size(); ++i)
			if (!renderObjects[i].material->IsTransparent())
				opaqueIdx.push_back(i);

		std::sort(opaqueIdx.begin(), opaqueIdx.end(),
			[&](size_t a, size_t b) {
				return renderObjects[a].material.get() < renderObjects[b].material.get();
			});

		std::vector<DrawIndexedIndirectCommand> commands(opaqueIdx.size());
		struct MaterialRange {
			std::shared_ptr<Material> material;
			uint32_t start, count;
		};
		std::vector<MaterialRange> ranges;
		{
			size_t j = 0;
			while (j < opaqueIdx.size())
			{
				auto* currentMat = renderObjects[opaqueIdx[j]].material.get();
				size_t start = j;
				while (j < opaqueIdx.size() && renderObjects[opaqueIdx[j]].material.get() == currentMat)
					++j;
				ranges.push_back({ renderObjects[opaqueIdx[start]].material,
					static_cast<uint32_t>(start), static_cast<uint32_t>(j - start) });
			}
		}
		for (size_t i = 0; i < opaqueIdx.size(); ++i)
		{
			auto& obj = renderObjects[opaqueIdx[i]];
			commands[i] = {
				obj.mesh->GetIndexCount(), 1,
				obj.mesh->GetGlobalFirstIndex(),
				obj.mesh->GetGlobalVertexOffset(),
				static_cast<uint32_t>(opaqueIdx[i])
			};
		}

		auto& indirectBuf = m_IndirectBuffer[currentFrameIndex];
		if (!commands.empty())
		{
			size_t cmdBytes = commands.size() * sizeof(DrawIndexedIndirectCommand);
			if (indirectBuf->GetSize() < cmdBytes)
			{
				m_Context->device->WaitIdle(); // old buffer may still be referenced by in-flight frames
				indirectBuf.reset();
				indirectBuf = m_Context->device->CreateBuffer(
					cmdBytes + sizeof(DrawIndexedIndirectCommand) * 64,
					BufferUsage::IndirectBuffer | BufferUsage::TransferDstBuffer, true);
			}
			indirectBuf->UploadData(commands.data(), cmdBytes, 0);
		}

		// single forward pass: color + depth (depth cleared here, no separate PreZ)
		cmd->BeginRenderPass(*m_PbrClearRenderPass, *m_PbrClearFramebuffers[currentImageIndex], width, height, { {{}, {}, false}, {{}, {}, true} });
		cmd->SetScissor(0, 0, width, height);
		cmd->SetViewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		if (!commands.empty())
		{
			cmd->BindVertexBuffer(*Mesh::GetGlobalVertexBuffer(), 0, 0);
			cmd->BindIndexBuffer(*Mesh::GetGlobalIndexBuffer(), 0);
			for (auto& range : ranges)
			{
				cmd->BindPipeline(*m_PbrPipeline);
				cmd->BindDescriptorSet(*m_PbrPipelineLayout, *m_Context->baseDataDescriptorSet[currentFrameIndex], 0);
				cmd->BindDescriptorSet(*m_PbrPipelineLayout, *m_Context->sceneDataDescriptorSet[currentFrameIndex], 1);
				cmd->BindDescriptorSet(*m_PbrPipelineLayout, *range.material->GetDescriptorSet(), 2);
				cmd->DrawIndexedIndirect(*indirectBuf, range.count,
					sizeof(DrawIndexedIndirectCommand), range.start * sizeof(DrawIndexedIndirectCommand));
			}
		}
		cmd->EndRenderPass();

		m_Stats.opaqueObjects = static_cast<uint32_t>(commands.size());
		m_Stats.drawCalls = commands.empty() ? 0 : static_cast<uint32_t>(ranges.size());
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
