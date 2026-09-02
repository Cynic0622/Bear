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
		CreateRenderPass();
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

	void PbrPass::Execute(RHICommandList* cmd, std::vector<RenderObject> renderObjects)
	{
		auto currentImageIndex = m_Context->device->GetCurrentImageIndex();
		auto currentFrameIndex = m_Context->device->GetCurrentFrameIndex();
		auto width = m_Context->swapchain->GetWidth();
		auto height = m_Context->swapchain->GetHeight();

		// Resolve the command pool: CullingPass builds it when present; otherwise build locally.
		const std::vector<DrawIndexedIndirectCommand>* commands = nullptr;
		const std::vector<CullingMaterialRange>* ranges = nullptr;
		const std::vector<uint32_t>* regionBases = nullptr;
		std::vector<DrawIndexedIndirectCommand> localCommands;
		std::vector<CullingMaterialRange> localRanges;
		std::vector<uint32_t> localRegionBases;
		uint32_t objectCount = 0;


		if (m_CullingPass)
		{
			commands = &m_CullingPass->GetCommands();
			ranges = &m_CullingPass->GetMaterialRanges();
			regionBases = &m_CullingPass->GetRegionBases();
			objectCount = m_CullingPass->GetObjectCount();

		}
		else
		{
			std::vector<size_t> opaqueIdx;
			for (size_t i = 0; i < renderObjects.size(); ++i)
				if (!renderObjects[i].material->IsTransparent())
					opaqueIdx.push_back(i);

			std::sort(opaqueIdx.begin(), opaqueIdx.end(),
				[&](size_t a, size_t b) {
					return renderObjects[a].material.get() < renderObjects[b].material.get();
				});

			localCommands.resize(opaqueIdx.size());
			uint32_t materialIndex = 0;
			localRanges.push_back({ 0, 0, 0, nullptr });
			localRegionBases.push_back(0);
			for (size_t i = 0; i < opaqueIdx.size(); ++i)
			{
				auto& obj = renderObjects[opaqueIdx[i]];
				localCommands[i] = {
					obj.mesh->GetIndexCount(), 1,
					obj.mesh->GetGlobalFirstIndex(),
					obj.mesh->GetGlobalVertexOffset(),
					static_cast<uint32_t>(opaqueIdx[i])
				};
				if (i > 0 && renderObjects[opaqueIdx[i]].material.get() != renderObjects[opaqueIdx[i - 1]].material.get())
				{
					++materialIndex;
					localRanges.push_back({ materialIndex, static_cast<uint32_t>(i), 0, renderObjects[opaqueIdx[i]].material });
					localRegionBases.push_back(static_cast<uint32_t>(i));
				}
				localRanges.back().count++;
			}
			commands = &localCommands;
			ranges = &localRanges;
			regionBases = &localRegionBases;
			objectCount = static_cast<uint32_t>(opaqueIdx.size());

		}

		bool gpuCulling = m_CullingPass && m_CullingPass->IsEnabled() && objectCount > 0;

		// CPU path: upload the pool into our own indirect buffer.
		auto& indirectBuf = m_IndirectBuffer[currentFrameIndex];
		if (!gpuCulling && objectCount > 0)
		{
			size_t cmdBytes = commands->size() * sizeof(DrawIndexedIndirectCommand);
			if (indirectBuf->GetSize() < cmdBytes)
			{
				indirectBuf.reset();
				indirectBuf = m_Context->device->CreateBuffer(
					cmdBytes + sizeof(DrawIndexedIndirectCommand) * 64,
					BufferUsage::IndirectBuffer | BufferUsage::TransferDstBuffer, true);
			}
			indirectBuf->UploadData(commands->data(), cmdBytes, 0);
		}

		std::vector<RHIClearValue> clearValues = { {{0.1f, 0.1f, 0.1f}, {}, false}, {{}, {}, true} };
		cmd->BeginRenderPass(*m_RenderPass, *m_Framebuffers[currentImageIndex], width, height, clearValues);
		cmd->SetScissor(0, 0, width, height);
		cmd->SetViewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);

		cmd->BindPipeline(*m_PreZPipeline);
		cmd->BindDescriptorSet(*m_PreZPipelineLayout, *m_Context->baseDataDescriptorSet[currentFrameIndex], 0);
		cmd->BindDescriptorSet(*m_PreZPipelineLayout, *m_Context->sceneDataDescriptorSet[currentFrameIndex], 1);

		if (objectCount > 0)
		{
			cmd->BindVertexBuffer(*Mesh::GetGlobalVertexBuffer(), 0, 0);
			cmd->BindIndexBuffer(*Mesh::GetGlobalIndexBuffer(), 0);
			if (gpuCulling)
			{
				// output is bucketed per material: draw each bucket with its own counter
				for (auto& range : *ranges)
				{
					cmd->DrawIndexedIndirectCount(*m_CullingPass->GetOutputCommandsBuffer(),
						*m_CullingPass->GetCountersBuffer(), range.count,
						sizeof(DrawIndexedIndirectCommand),
						(*regionBases)[range.materialIndex] * sizeof(DrawIndexedIndirectCommand),
						range.materialIndex * sizeof(uint32_t));
				}
			}
			else
			{
				cmd->DrawIndexedIndirect(*indirectBuf, objectCount, sizeof(DrawIndexedIndirectCommand), 0);
			}
		}

		cmd->NextSubpass();

		for (auto& range : *ranges)
		{
			cmd->BindPipeline(*m_PbrPipeline);
			cmd->BindDescriptorSet(*m_PbrPipelineLayout, *m_Context->baseDataDescriptorSet[currentFrameIndex], 0);
			cmd->BindDescriptorSet(*m_PbrPipelineLayout, *m_Context->sceneDataDescriptorSet[currentFrameIndex], 1);
			cmd->BindDescriptorSet(*m_PbrPipelineLayout, *range.material->GetDescriptorSet(), 2);
			if (gpuCulling)
			{
				cmd->DrawIndexedIndirectCount(*m_CullingPass->GetOutputCommandsBuffer(),
					*m_CullingPass->GetCountersBuffer(), range.count,
					sizeof(DrawIndexedIndirectCommand),
					(*regionBases)[range.materialIndex] * sizeof(DrawIndexedIndirectCommand),
					range.materialIndex * sizeof(uint32_t));
			}
			else
			{
				cmd->DrawIndexedIndirect(*indirectBuf, range.count,
					sizeof(DrawIndexedIndirectCommand), range.start * sizeof(DrawIndexedIndirectCommand));
			}
		}

		cmd->EndRenderPass();

		m_Stats.opaqueObjects = objectCount;
		m_Stats.drawCalls = (objectCount == 0 ? 0 : 1) + static_cast<uint32_t>(ranges->size());
	}

	void PbrPass::Resize()
	{
		m_Context->device->WaitIdle();
		Cleanup();
		CreateFramebuffers();
	}

	void PbrPass::Cleanup()
	{
		m_Framebuffers.clear();
	}

	void PbrPass::CreateRenderPass()
	{
		RenderPassDescription desc;
		desc.attachmentCount = 2; // 1 for depth, 1 for color
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
		depthAttachment.initialLayout = ImageLayout::DepthStencilAttachment;
		depthAttachment.finalLayout = ImageLayout::DepthStencilAttachment;

		desc.subpassCount = 2; // One for pre-Z pass, one for PBR pass
		SubpassDescription& preZSubpass = desc.subpasses[0];
		preZSubpass.hasDepthStencil = true;
		preZSubpass.depthStencilAttachment = {1, ImageLayout::DepthStencilAttachment};

		SubpassDescription& pbrSubpass = desc.subpasses[1];
		pbrSubpass.colorAttachmentCount = 1;
		pbrSubpass.colorAttachments[0] = {0, ImageLayout::ColorAttachment};
		pbrSubpass.hasDepthStencil = true;
		pbrSubpass.depthStencilAttachment = { 1, ImageLayout::DepthStencilAttachment };

		desc.dependencyCount = 1;
		SubpassDependency& dependency = desc.dependencies[0];
		dependency.srcSubpass = 0;
		dependency.dstSubpass = 1;
		dependency.srcStageMask = PipelineStage::LateFragmentTests | PipelineStage::EarlyFragmentTests;
		dependency.dstStageMask = PipelineStage::LateFragmentTests | PipelineStage::EarlyFragmentTests;
		dependency.srcAccessMask = AccessFlags::DepthStencilAttachmentWrite;
		dependency.dstAccessMask = AccessFlags::DepthStencilAttachmentRead;

		m_RenderPass = m_Context->device->CreateRenderPass(desc);
	}

	void PbrPass::CreateFramebuffers()
	{
		uint32_t width = m_Context->swapchain->GetWidth();
		uint32_t height = m_Context->swapchain->GetHeight();
		auto depthAttachment = m_Context->swapchain->GetDepthView();
		for (uint32_t i = 0; i < m_Context->swapchain->GetImageCount(); ++i)
		{
			m_Framebuffers.push_back(m_Context->device->CreateFramebuffer(*m_RenderPass, { m_Context->swapchain->GetColorView(i), depthAttachment }, width, height));
		}
	}

	void PbrPass::CreatePipeline()
	{
		std::vector<RHIPushConstantRange> pushConstantRanges = {
			{ShaderStage::Vertex, sizeof(PerObjectPushConstants), 0}
		};
		const auto& baseDataDescriptorSetLayout = m_Context->baseDataDescriptorSetLayout;
		const auto& sceneDataDescriptorSetLayout = m_Context->sceneDataDescriptorSetLayout;
		m_PreZPipelineLayout = m_Context->device->CreatePipelineLayout({ baseDataDescriptorSetLayout, sceneDataDescriptorSetLayout, m_DescriptorSetLayout.get()},
		                                                              { pushConstantRanges });

		RHIPipelineConfig pipelineConfig;
		pipelineConfig.pipelineLayout = m_PreZPipelineLayout;
		pipelineConfig.vertexShaderPath = "Bear/src/Shaders/preZvert.spv";
		pipelineConfig.fragmentShaderPath = "Bear/src/Shaders/preZfrag.spv";
		pipelineConfig.colorBlendAttachmentState.colorWriteMask = ColorWriteMask::None;
		pipelineConfig.depthStencilState.depthTestEnable = true;
		pipelineConfig.depthStencilState.depthWriteEnable = false;
		pipelineConfig.depthStencilState.depthCompareOp = CompareOp::Less;
		pipelineConfig.subpassIndex = 0;
		m_PreZPipeline = m_Context->device->CreatePipeline(pipelineConfig, *m_RenderPass);

		m_PbrPipelineLayout = m_Context->device->CreatePipelineLayout({ baseDataDescriptorSetLayout, sceneDataDescriptorSetLayout, m_DescriptorSetLayout.get()},
			{ pushConstantRanges });

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
		pipelineConfig.depthStencilState.depthWriteEnable = true;
		pipelineConfig.depthStencilState.depthCompareOp = CompareOp::LessOrEqual;
		pipelineConfig.subpassIndex = 1;
		m_PbrPipeline = m_Context->device->CreatePipeline(pipelineConfig, *m_RenderPass);
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
