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
		CreatePbrDescriptorSetLayout();
		CreatePipeline();
	}

	void PbrPass::Execute(RHICommandList* cmd, const IndirectDrawSet& drawSet, bool clearDepth, RHIImage* color, RHIImage* depth)
	{
		auto currentFrameIndex = m_Context->device->GetCurrentFrameIndex();
		auto width = m_Context->swapchain->GetWidth();
		auto height = m_Context->swapchain->GetHeight();

		if (!drawSet.commands || !drawSet.counters || !drawSet.ranges || !drawSet.regionBases)
			return;

		RHIClearValue depthClear{};
		depthClear.isDepth = true;
		depthClear.depthStencil.depth = 1.0f;

		std::vector<RHIRenderingAttachment> attachments = {
			{ color, ImageLayout::ColorAttachment, AttachmentLoadOp::Load, AttachmentStoreOp::Store, nullptr, false },
			{ depth, ImageLayout::DepthStencilAttachment, clearDepth ? AttachmentLoadOp::Clear : AttachmentLoadOp::Load, AttachmentStoreOp::Store, clearDepth ? &depthClear : nullptr, true }
		};

		cmd->BeginRendering(attachments);
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

		cmd->EndRendering();

		m_Stats.opaqueObjects += drawSet.objectCount;
		m_Stats.drawCalls += static_cast<uint32_t>(ranges.size());
	}

	void PbrPass::Cleanup()
	{
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
		pipelineConfig.dynamicRendering = true;
		pipelineConfig.colorFormats = { PixelFormat::B8G8R8A8_SRGB };
		pipelineConfig.depthFormat = PixelFormat::D32_SFLOAT;
		m_PbrPipeline = m_Context->device->CreatePipeline(pipelineConfig);
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
