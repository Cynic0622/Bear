#include "bearpch.h"
#include "CullingPass.h"
#include "RHI/RHIDevice.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIPipeline.h"
#include "RHI/RHIResources.h"
#include "Common/RenderObject.h"
#include "Scene/Frustum.h"

namespace Bear
{
	namespace
	{
		constexpr uint32_t kGroupSize = 64;
		constexpr float kDepthBias = 0.0001f;

		// must match cull.comp push constants layout
		struct CullPushConstants
		{
			glm::vec4 planes[6];
			uint32_t objectCount;
			uint32_t materialCount;
			uint32_t hizMipCount;
			uint32_t occlusionEnabled;
			float depthBias;
		};
		static_assert(sizeof(CullPushConstants) <= 128, "push constants exceed the 128-byte limit");

		// must match cull2.comp push constants layout
		struct Cull2PushConstants
		{
			glm::vec4 planes[6];
			uint32_t maxCount;
			uint32_t materialCount;
			uint32_t hizMipCount;
			float depthBias;
		};

		struct RejectedCommand
		{
			DrawIndexedIndirectCommand cmd;
			uint32_t materialId;
		};
	}

	void CullingPass::Setup(RenderContext* context)
	{
		m_Context = context;

		m_DescriptorSetLayout = m_Context->device->CreateDescriptorSetLayout({
			{0, DescriptorType::StorageBuffer, 1, ShaderStage::Compute }, // AABBs
			{1, DescriptorType::StorageBuffer, 1, ShaderStage::Compute }, // input commands
			{2, DescriptorType::StorageBuffer, 1, ShaderStage::Compute }, // output A
			{3, DescriptorType::StorageBuffer, 1, ShaderStage::Compute }, // counters A
			{4, DescriptorType::StorageBuffer, 1, ShaderStage::Compute }, // material ids
			{5, DescriptorType::StorageBuffer, 1, ShaderStage::Compute }, // region bases
			{6, DescriptorType::StorageBuffer, 1, ShaderStage::Compute }, // rejected commands
			{7, DescriptorType::StorageBuffer, 1, ShaderStage::Compute }, // rejected counter
			{8, DescriptorType::CombinedImageSampler, 1, ShaderStage::Compute }, // Hi-Z
			{9, DescriptorType::StorageBuffer, 1, ShaderStage::Compute }, // totals
			});

		m_PipelineLayout = m_Context->device->CreatePipelineLayout(
			{ m_Context->baseDataDescriptorSetLayout, m_Context->sceneDataDescriptorSetLayout, m_DescriptorSetLayout.get() },
			{ { ShaderStage::Compute, sizeof(CullPushConstants), 0 } });

		m_Phase2DescriptorSetLayout = m_Context->device->CreateDescriptorSetLayout({
			{0, DescriptorType::StorageBuffer, 1, ShaderStage::Compute }, // AABBs
			{1, DescriptorType::StorageBuffer, 1, ShaderStage::Compute }, // rejected commands
			{2, DescriptorType::StorageBuffer, 1, ShaderStage::Compute }, // output B
			{3, DescriptorType::StorageBuffer, 1, ShaderStage::Compute }, // counters B
			{4, DescriptorType::StorageBuffer, 1, ShaderStage::Compute }, // rejected counter
			{5, DescriptorType::StorageBuffer, 1, ShaderStage::Compute }, // region bases
			{6, DescriptorType::CombinedImageSampler, 1, ShaderStage::Compute }, // Hi-Z
			{7, DescriptorType::StorageBuffer, 1, ShaderStage::Compute }, // totals
			});

		m_Phase2PipelineLayout = m_Context->device->CreatePipelineLayout(
			{ m_Context->baseDataDescriptorSetLayout, m_Context->sceneDataDescriptorSetLayout, m_Phase2DescriptorSetLayout.get() },
			{ { ShaderStage::Compute, sizeof(Cull2PushConstants), 0 } });

		CreatePipelines();

		m_DescriptorSets.resize(m_Context->MAX_FRAMES_IN_FLIGHT);
		m_Phase2DescriptorSets.resize(m_Context->MAX_FRAMES_IN_FLIGHT);
		m_AabbBuffers.resize(m_Context->MAX_FRAMES_IN_FLIGHT);
		m_InputCommandsBuffers.resize(m_Context->MAX_FRAMES_IN_FLIGHT);
		m_OutputCommandsBuffers.resize(m_Context->MAX_FRAMES_IN_FLIGHT);
		m_CountersBuffers.resize(m_Context->MAX_FRAMES_IN_FLIGHT);
		m_MaterialIdBuffers.resize(m_Context->MAX_FRAMES_IN_FLIGHT);
		m_RegionBaseBuffers.resize(m_Context->MAX_FRAMES_IN_FLIGHT);
		m_RejectedBuffers.resize(m_Context->MAX_FRAMES_IN_FLIGHT);
		m_RejectedCountersBuffers.resize(m_Context->MAX_FRAMES_IN_FLIGHT);
		m_RescuedCommandsBuffers.resize(m_Context->MAX_FRAMES_IN_FLIGHT);
		m_RescuedCountersBuffers.resize(m_Context->MAX_FRAMES_IN_FLIGHT);
		m_TotalsBuffers.resize(m_Context->MAX_FRAMES_IN_FLIGHT);

		for (uint8_t i = 0; i < m_Context->MAX_FRAMES_IN_FLIGHT; ++i)
		{
			m_DescriptorSets[i] = m_Context->device->CreateDescriptorSet(m_DescriptorSetLayout);
			m_Phase2DescriptorSets[i] = m_Context->device->CreateDescriptorSet(m_Phase2DescriptorSetLayout);
		}
	}

	void CullingPass::CreatePipelines()
	{
		RHIPipelineConfig config;
		config.pipelineLayout = m_PipelineLayout;
		config.computeShaderPath = "Bear/src/Shaders/cullPhase1.spv";
		m_Pipeline = m_Context->device->CreateComputePipeline(config);

		config.pipelineLayout = m_Phase2PipelineLayout;
		config.computeShaderPath = "Bear/src/Shaders/cullPhase2.spv";
		m_Phase2Pipeline = m_Context->device->CreateComputePipeline(config);
	}

	void CullingPass::Cleanup()
	{
		m_Pipeline.reset();
		m_PipelineLayout.reset();
		m_DescriptorSetLayout.reset();
		m_Phase2Pipeline.reset();
		m_Phase2PipelineLayout.reset();
		m_Phase2DescriptorSetLayout.reset();
		m_DescriptorSets.clear();
		m_Phase2DescriptorSets.clear();
		m_AabbBuffers.clear();
		m_InputCommandsBuffers.clear();
		m_OutputCommandsBuffers.clear();
		m_CountersBuffers.clear();
		m_MaterialIdBuffers.clear();
		m_RegionBaseBuffers.clear();
		m_RejectedBuffers.clear();
		m_RejectedCountersBuffers.clear();
		m_RescuedCommandsBuffers.clear();
		m_RescuedCountersBuffers.clear();
	}

	void CullingPass::SetViewProjection(const glm::mat4& viewProjection)
	{
		const glm::mat4& m = viewProjection;
		m_Planes[0] = glm::vec4(m[0][3] + m[0][0], m[1][3] + m[1][0], m[2][3] + m[2][0], m[3][3] + m[3][0]); // left
		m_Planes[1] = glm::vec4(m[0][3] - m[0][0], m[1][3] - m[1][0], m[2][3] - m[2][0], m[3][3] - m[3][0]); // right
		m_Planes[2] = glm::vec4(m[0][3] + m[0][1], m[1][3] + m[1][1], m[2][3] + m[2][1], m[3][3] + m[3][1]); // bottom
		m_Planes[3] = glm::vec4(m[0][3] - m[0][1], m[1][3] - m[1][1], m[2][3] - m[2][1], m[3][3] - m[3][1]); // top
		m_Planes[4] = glm::vec4(m[0][3] + m[0][2], m[1][3] + m[1][2], m[2][3] + m[2][2], m[3][3] + m[3][2]); // near
		m_Planes[5] = glm::vec4(m[0][3] - m[0][2], m[1][3] - m[1][2], m[2][3] - m[2][2], m[3][3] - m[3][2]); // far

		for (int i = 0; i < 6; ++i)
		{
			float len = glm::length(glm::vec3(m_Planes[i]));
			if (len > 0.0f)
				m_Planes[i] /= len;
		}
	}

	void CullingPass::EnsureBuffers(uint32_t objectCount, uint32_t materialCount)
	{
		size_t cmdBytes = objectCount * sizeof(DrawIndexedIndirectCommand);
		size_t aabbBytes = objectCount * sizeof(AabbUpload);
		size_t counterBytes = (materialCount + 1) * sizeof(uint32_t);
		size_t idBytes = objectCount * sizeof(uint32_t);
		size_t regionBytes = materialCount * sizeof(uint32_t);
		size_t rejectedBytes = objectCount * sizeof(RejectedCommand);

		auto recreate = [&](std::unique_ptr<RHIBuffer>& buf, size_t size, BufferUsage usage)
		{
			if (!buf || buf->GetSize() < size)
			{
				buf = m_Context->device->CreateBuffer(size, usage, true);
			}
		};

		// if anything needs reallocation, wait for the GPU first: in-flight frames may still
		// reference the old buffers through their descriptor sets
		auto insufficient = [](const std::unique_ptr<RHIBuffer>& buf, size_t size) { return !buf || buf->GetSize() < size; };
		bool needsRecreate = false;
		for (uint8_t i = 0; i < m_Context->MAX_FRAMES_IN_FLIGHT && !needsRecreate; ++i)
		{
			needsRecreate =
				insufficient(m_AabbBuffers[i], aabbBytes) ||
				insufficient(m_InputCommandsBuffers[i], cmdBytes) ||
				insufficient(m_OutputCommandsBuffers[i], cmdBytes) ||
				insufficient(m_CountersBuffers[i], counterBytes) ||
				insufficient(m_MaterialIdBuffers[i], idBytes) ||
				insufficient(m_RegionBaseBuffers[i], regionBytes) ||
				insufficient(m_RejectedBuffers[i], rejectedBytes) ||
				insufficient(m_RejectedCountersBuffers[i], sizeof(uint32_t)) ||
				insufficient(m_RescuedCommandsBuffers[i], cmdBytes) ||
				insufficient(m_RescuedCountersBuffers[i], counterBytes) ||
				insufficient(m_TotalsBuffers[i], sizeof(uint32_t) * 3);
		}
		if (needsRecreate)
		{
			m_Context->device->WaitIdle();
		}

		for (uint8_t i = 0; i < m_Context->MAX_FRAMES_IN_FLIGHT; ++i)
		{
			recreate(m_AabbBuffers[i], aabbBytes, BufferUsage::StorageBuffer | BufferUsage::TransferDstBuffer);
			recreate(m_InputCommandsBuffers[i], cmdBytes, BufferUsage::StorageBuffer | BufferUsage::TransferDstBuffer);
			recreate(m_OutputCommandsBuffers[i], cmdBytes, BufferUsage::StorageBuffer | BufferUsage::IndirectBuffer | BufferUsage::TransferDstBuffer);
			recreate(m_CountersBuffers[i], counterBytes, BufferUsage::StorageBuffer | BufferUsage::IndirectBuffer | BufferUsage::TransferDstBuffer);
			recreate(m_MaterialIdBuffers[i], idBytes, BufferUsage::StorageBuffer | BufferUsage::TransferDstBuffer);
			recreate(m_RegionBaseBuffers[i], regionBytes, BufferUsage::StorageBuffer | BufferUsage::TransferDstBuffer);
			recreate(m_RejectedBuffers[i], rejectedBytes, BufferUsage::StorageBuffer | BufferUsage::TransferDstBuffer);
			recreate(m_RejectedCountersBuffers[i], sizeof(uint32_t), BufferUsage::StorageBuffer | BufferUsage::TransferDstBuffer);
			recreate(m_RescuedCommandsBuffers[i], cmdBytes, BufferUsage::StorageBuffer | BufferUsage::IndirectBuffer | BufferUsage::TransferDstBuffer);
			recreate(m_RescuedCountersBuffers[i], counterBytes, BufferUsage::StorageBuffer | BufferUsage::IndirectBuffer | BufferUsage::TransferDstBuffer);
			recreate(m_TotalsBuffers[i], sizeof(uint32_t) * 3, BufferUsage::StorageBuffer | BufferUsage::TransferDstBuffer);
		}
	}

	void CullingPass::BuildCommandPool(const std::vector<RenderObject>& renderObjects)
	{
		m_Commands.clear();
		m_MaterialRanges.clear();
		m_RegionBases.clear();
		m_MaterialIds.clear();

		std::vector<size_t> opaqueIdx;
		opaqueIdx.reserve(renderObjects.size());
		for (size_t i = 0; i < renderObjects.size(); ++i)
		{
			if (!renderObjects[i].material->IsTransparent())
				opaqueIdx.push_back(i);
		}

		std::sort(opaqueIdx.begin(), opaqueIdx.end(),
			[&](size_t a, size_t b) {
				return renderObjects[a].material.get() < renderObjects[b].material.get();
			});

		if (opaqueIdx.empty())
		{
			m_Commands.clear();
			m_MaterialIds.clear();
			m_MaterialRanges.clear();
			m_RegionBases.clear();
			m_ObjectCount = 0;
			m_MaterialCount = 0;
			return;
		}

		m_Commands.resize(opaqueIdx.size());
		m_MaterialIds.resize(opaqueIdx.size());

		uint32_t materialIndex = 0;
		m_MaterialRanges.push_back({ 0, 0, 0, renderObjects[opaqueIdx[0]].material });
		m_RegionBases.push_back(0);

		for (size_t i = 0; i < opaqueIdx.size(); ++i)
		{
			auto& obj = renderObjects[opaqueIdx[i]];
			m_Commands[i] = {
				obj.mesh->GetIndexCount(),
				1,
				obj.mesh->GetGlobalFirstIndex(),
				obj.mesh->GetGlobalVertexOffset(),
				static_cast<uint32_t>(opaqueIdx[i])
			};

			if (i > 0 && renderObjects[opaqueIdx[i]].material.get() != renderObjects[opaqueIdx[i - 1]].material.get())
			{
				++materialIndex;
				m_MaterialRanges.push_back({ materialIndex, static_cast<uint32_t>(i), 0, renderObjects[opaqueIdx[i]].material });
				m_RegionBases.push_back(static_cast<uint32_t>(i));
			}
			m_MaterialIds[i] = materialIndex;
			m_MaterialRanges.back().count++;
		}

		m_ObjectCount = static_cast<uint32_t>(opaqueIdx.size());
		m_MaterialCount = static_cast<uint32_t>(m_MaterialRanges.size());
	}

	void CullingPass::UpdateAabbBuffer(const std::vector<RenderObject>& renderObjects)
	{
		bool changed = m_CachedMeshes.size() != renderObjects.size();
		if (!changed)
		{
			for (size_t i = 0; i < renderObjects.size(); ++i)
			{
				if (m_CachedMeshes[i] != renderObjects[i].mesh.get())
				{
					changed = true;
					break;
				}
			}
		}
		if (!changed)
			return;

		m_CachedMeshes.resize(renderObjects.size());
		m_AabbData.resize(renderObjects.size());
		for (size_t i = 0; i < renderObjects.size(); ++i)
		{
			m_CachedMeshes[i] = renderObjects[i].mesh.get();
			const AABB& aabb = renderObjects[i].mesh->GetAABB();
			m_AabbData[i] = { glm::vec4(aabb.min, 0.0f), glm::vec4(aabb.max, 0.0f) };
		}

		// upload to every in-flight frame's buffer, otherwise frames alternate between
		// correct data and uninitialized memory -> visible flicker
		for (uint8_t i = 0; i < m_Context->MAX_FRAMES_IN_FLIGHT; ++i)
		{
			m_AabbBuffers[i]->UploadData(m_AabbData.data(), m_AabbData.size() * sizeof(AabbUpload), 0);
		}
	}

	void CullingPass::Execute(RHICommandList* cmd, const std::vector<RenderObject>& renderObjects)
	{
		m_FrameIndex = m_Context->device->GetCurrentFrameIndex();

		BuildCommandPool(renderObjects);
		if (m_ObjectCount == 0 || !m_Enabled)
			return;

		EnsureBuffers(m_ObjectCount, m_MaterialCount);
		UpdateAabbBuffer(renderObjects);

		// read last time this frame slot ran (its fence has already signaled) for the UI stats
		{
			uint32_t totals[3] = { 0, 0, 0 };
			m_TotalsBuffers[m_FrameIndex]->ReadData(totals, sizeof(totals), 0);
			m_GpuStats.visibleA = totals[0];
			m_GpuStats.rejected = totals[1];
			m_GpuStats.rescued = totals[2];
		}

		// CPU-side uploads (same host-coherent path as PbrPass indirect commands)
		m_InputCommandsBuffers[m_FrameIndex]->UploadData(m_Commands.data(), m_Commands.size() * sizeof(DrawIndexedIndirectCommand), 0);
		m_MaterialIdBuffers[m_FrameIndex]->UploadData(m_MaterialIds.data(), m_MaterialIds.size() * sizeof(uint32_t), 0);
		m_RegionBaseBuffers[m_FrameIndex]->UploadData(m_RegionBases.data(), m_RegionBases.size() * sizeof(uint32_t), 0);

		// Keep the descriptor set bindings in sync in case buffers were recreated
		auto& set = m_DescriptorSets[m_FrameIndex];
		set->UpdateBuffer(0, *m_AabbBuffers[m_FrameIndex]);
		set->UpdateBuffer(1, *m_InputCommandsBuffers[m_FrameIndex]);
		set->UpdateBuffer(2, *m_OutputCommandsBuffers[m_FrameIndex]);
		set->UpdateBuffer(3, *m_CountersBuffers[m_FrameIndex]);
		set->UpdateBuffer(4, *m_MaterialIdBuffers[m_FrameIndex]);
		set->UpdateBuffer(5, *m_RegionBaseBuffers[m_FrameIndex]);
		set->UpdateBuffer(6, *m_RejectedBuffers[m_FrameIndex]);
		set->UpdateBuffer(7, *m_RejectedCountersBuffers[m_FrameIndex]);
		set->UpdateBuffer(9, *m_TotalsBuffers[m_FrameIndex]);

		// binding 8 is statically used by the shader, so it must always be valid
		if (m_HiZImage && m_HiZSampler)
		{
			set->UpdateSampledImage(8, *m_HiZImage, *m_HiZSampler, UINT32_MAX, ImageLayout::General);
		}

		bool occlusion = OcclusionActive();

		// zero counters: A[M+1], rejected[1] and totals[3]
		{
			std::vector<uint32_t> zerosA(m_MaterialCount + 1, 0);
			cmd->FillBuffer(*m_CountersBuffers[m_FrameIndex], zerosA.data(), zerosA.size() * sizeof(uint32_t), 0);
			uint32_t zero = 0;
			cmd->FillBuffer(*m_RejectedCountersBuffers[m_FrameIndex], &zero, sizeof(zero), 0);
			uint32_t zeros3[3] = { 0, 0, 0 };
			cmd->FillBuffer(*m_TotalsBuffers[m_FrameIndex], zeros3, sizeof(zeros3), 0);
		}

		MemoryBarrier fillBarrier;
		fillBarrier.srcStageMask = PipelineStage::Transfer;
		fillBarrier.dstStageMask = PipelineStage::ComputeShader;
		fillBarrier.srcAccessMask = AccessFlags::TransferWrite;
		fillBarrier.dstAccessMask = AccessFlags::ShaderRead | AccessFlags::ShaderWrite;
		cmd->PipelineBarrier(fillBarrier);

		cmd->BindPipeline(*m_Pipeline);
		cmd->BindDescriptorSet(*m_PipelineLayout, *m_Context->baseDataDescriptorSet[m_FrameIndex], 0, PipelineBindPoint::Compute);
		cmd->BindDescriptorSet(*m_PipelineLayout, *m_Context->sceneDataDescriptorSet[m_FrameIndex], 1, PipelineBindPoint::Compute);
		cmd->BindDescriptorSet(*m_PipelineLayout, *set, 2, PipelineBindPoint::Compute);

		CullPushConstants pc{};
		memcpy(pc.planes, m_Planes, sizeof(pc.planes));
		pc.objectCount = m_ObjectCount;
		pc.materialCount = m_MaterialCount;
		pc.hizMipCount = m_HiZMipCount;
		pc.occlusionEnabled = occlusion ? 1u : 0u;
		pc.depthBias = kDepthBias;
		cmd->PushConstants(*m_PipelineLayout, ShaderStage::Compute, &pc, sizeof(pc), 0);

		uint32_t groups = (m_ObjectCount + kGroupSize - 1) / kGroupSize;
		cmd->Dispatch(groups, 1, 1);

		// compute writes -> draw indirect reads
		MemoryBarrier barrier;
		barrier.srcStageMask = PipelineStage::ComputeShader;
		barrier.dstStageMask = PipelineStage::DrawIndirect;
		barrier.srcAccessMask = AccessFlags::ShaderWrite;
		barrier.dstAccessMask = AccessFlags::IndirectCommandRead;
		cmd->PipelineBarrier(barrier);
	}

	void CullingPass::ExecutePhase2(RHICommandList* cmd)
	{
		if (m_ObjectCount == 0 || !OcclusionActive())
			return;

		auto& set = m_Phase2DescriptorSets[m_FrameIndex];
		set->UpdateBuffer(0, *m_AabbBuffers[m_FrameIndex]);
		set->UpdateBuffer(1, *m_RejectedBuffers[m_FrameIndex]);
		set->UpdateBuffer(2, *m_RescuedCommandsBuffers[m_FrameIndex]);
		set->UpdateBuffer(3, *m_RescuedCountersBuffers[m_FrameIndex]);
		set->UpdateBuffer(4, *m_RejectedCountersBuffers[m_FrameIndex]);
		set->UpdateBuffer(5, *m_RegionBaseBuffers[m_FrameIndex]);
		set->UpdateBuffer(7, *m_TotalsBuffers[m_FrameIndex]);
		set->UpdateSampledImage(6, *m_HiZImage, *m_HiZSampler, UINT32_MAX, ImageLayout::General);

		// zero rescued counters
		std::vector<uint32_t> zerosB(m_MaterialCount + 1, 0);
		cmd->FillBuffer(*m_RescuedCountersBuffers[m_FrameIndex], zerosB.data(), zerosB.size() * sizeof(uint32_t), 0);

		MemoryBarrier fillBarrier;
		fillBarrier.srcStageMask = PipelineStage::Transfer;
		fillBarrier.dstStageMask = PipelineStage::ComputeShader;
		fillBarrier.srcAccessMask = AccessFlags::TransferWrite;
		fillBarrier.dstAccessMask = AccessFlags::ShaderRead | AccessFlags::ShaderWrite;
		cmd->PipelineBarrier(fillBarrier);

		cmd->BindPipeline(*m_Phase2Pipeline);
		cmd->BindDescriptorSet(*m_Phase2PipelineLayout, *m_Context->baseDataDescriptorSet[m_FrameIndex], 0, PipelineBindPoint::Compute);
		cmd->BindDescriptorSet(*m_Phase2PipelineLayout, *m_Context->sceneDataDescriptorSet[m_FrameIndex], 1, PipelineBindPoint::Compute);
		cmd->BindDescriptorSet(*m_Phase2PipelineLayout, *set, 2, PipelineBindPoint::Compute);

		Cull2PushConstants pc{};
		memcpy(pc.planes, m_Planes, sizeof(pc.planes));
		pc.maxCount = m_ObjectCount;
		pc.materialCount = m_MaterialCount;
		pc.hizMipCount = m_HiZMipCount;
		pc.depthBias = kDepthBias;
		cmd->PushConstants(*m_Phase2PipelineLayout, ShaderStage::Compute, &pc, sizeof(pc), 0);

		uint32_t groups = (m_ObjectCount + kGroupSize - 1) / kGroupSize;
		cmd->Dispatch(groups, 1, 1);

		MemoryBarrier barrier;
		barrier.srcStageMask = PipelineStage::ComputeShader;
		barrier.dstStageMask = PipelineStage::DrawIndirect;
		barrier.srcAccessMask = AccessFlags::ShaderWrite;
		barrier.dstAccessMask = AccessFlags::IndirectCommandRead;
		cmd->PipelineBarrier(barrier);
	}
}
