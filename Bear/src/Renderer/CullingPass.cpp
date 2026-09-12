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

		struct CullPushConstants
		{
			glm::vec4 planes[6];
			uint32_t objectCount;
			uint32_t materialCount;
		};
		static_assert(sizeof(CullPushConstants) == 104, "must stay within the 128-byte push constant limit");
	}

	void CullingPass::Setup(RenderContext* context)
	{
		m_Context = context;

		m_DescriptorSetLayout = m_Context->device->CreateDescriptorSetLayout({
			{0, DescriptorType::StorageBuffer, 1, ShaderStage::Compute }, // AABBs
			{1, DescriptorType::StorageBuffer, 1, ShaderStage::Compute }, // input commands
			{2, DescriptorType::StorageBuffer, 1, ShaderStage::Compute }, // output commands
			{3, DescriptorType::StorageBuffer, 1, ShaderStage::Compute }, // counters
			{4, DescriptorType::StorageBuffer, 1, ShaderStage::Compute }, // material ids
			{5, DescriptorType::StorageBuffer, 1, ShaderStage::Compute }, // region bases
			});

		m_PipelineLayout = m_Context->device->CreatePipelineLayout(
			{ m_Context->sceneDataDescriptorSetLayout, m_DescriptorSetLayout.get() },
			{ { ShaderStage::Compute, sizeof(CullPushConstants), 0 } });

		RHIPipelineConfig config;
		config.pipelineLayout = m_PipelineLayout;
		config.computeShaderPath = "Bear/src/Shaders/cull.spv";
		m_Pipeline = m_Context->device->CreateComputePipeline(config);

		m_DescriptorSets.resize(m_Context->MAX_FRAMES_IN_FLIGHT);
		m_AabbBuffers.resize(m_Context->MAX_FRAMES_IN_FLIGHT);
		m_InputCommandsBuffers.resize(m_Context->MAX_FRAMES_IN_FLIGHT);
		m_OutputCommandsBuffers.resize(m_Context->MAX_FRAMES_IN_FLIGHT);
		m_CountersBuffers.resize(m_Context->MAX_FRAMES_IN_FLIGHT);
		m_MaterialIdBuffers.resize(m_Context->MAX_FRAMES_IN_FLIGHT);
		m_RegionBaseBuffers.resize(m_Context->MAX_FRAMES_IN_FLIGHT);

		for (uint8_t i = 0; i < m_Context->MAX_FRAMES_IN_FLIGHT; ++i)
		{
			m_DescriptorSets[i] = m_Context->device->CreateDescriptorSet(m_DescriptorSetLayout);
		}
	}

	void CullingPass::Cleanup()
	{
		m_Pipeline.reset();
		m_PipelineLayout.reset();
		m_DescriptorSetLayout.reset();
		m_DescriptorSets.clear();
		m_AabbBuffers.clear();
		m_InputCommandsBuffers.clear();
		m_OutputCommandsBuffers.clear();
		m_CountersBuffers.clear();
		m_MaterialIdBuffers.clear();
		m_RegionBaseBuffers.clear();
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

	RHIBuffer* CullingPass::GetOutputCommandsBuffer() const
	{
		return m_OutputCommandsBuffers[m_FrameIndex].get();
	}

	RHIBuffer* CullingPass::GetCountersBuffer() const
	{
		return m_CountersBuffers[m_FrameIndex].get();
	}

	void CullingPass::EnsureBuffers(uint32_t objectCount, uint32_t materialCount)
	{
		size_t cmdBytes = objectCount * sizeof(DrawIndexedIndirectCommand);
		size_t aabbBytes = objectCount * sizeof(AabbUpload);
		size_t counterBytes = (materialCount + 1) * sizeof(uint32_t);
		size_t idBytes = objectCount * sizeof(uint32_t);
		size_t regionBytes = materialCount * sizeof(uint32_t);

		auto recreate = [&](std::unique_ptr<RHIBuffer>& buf, size_t size, BufferUsage usage)
		{
			if (!buf || buf->GetSize() < size)
			{
				buf = m_Context->device->CreateBuffer(size, usage, true);
			}
		};

		for (uint8_t i = 0; i < m_Context->MAX_FRAMES_IN_FLIGHT; ++i)
		{
			recreate(m_AabbBuffers[i], aabbBytes, BufferUsage::StorageBuffer | BufferUsage::TransferDstBuffer);
			recreate(m_InputCommandsBuffers[i], cmdBytes, BufferUsage::StorageBuffer | BufferUsage::TransferDstBuffer);
			recreate(m_OutputCommandsBuffers[i], cmdBytes, BufferUsage::StorageBuffer | BufferUsage::IndirectBuffer | BufferUsage::TransferDstBuffer);
			recreate(m_CountersBuffers[i], counterBytes, BufferUsage::StorageBuffer | BufferUsage::IndirectBuffer | BufferUsage::TransferDstBuffer);
			recreate(m_MaterialIdBuffers[i], idBytes, BufferUsage::StorageBuffer | BufferUsage::TransferDstBuffer);
			recreate(m_RegionBaseBuffers[i], regionBytes, BufferUsage::StorageBuffer | BufferUsage::TransferDstBuffer);
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
		if (m_ObjectCount == 0)
			return;

		EnsureBuffers(m_ObjectCount, m_MaterialCount);
		UpdateAabbBuffer(renderObjects);

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

		if (!m_Enabled)
			return;

		// zero counters: [0..M-1] per-material, [M] total visible count
		std::vector<uint32_t> zeros(m_MaterialCount + 1, 0);
		cmd->FillBuffer(*m_CountersBuffers[m_FrameIndex], zeros.data(), zeros.size() * sizeof(uint32_t), 0);

		MemoryBarrier fillBarrier;
		fillBarrier.srcStageMask = PipelineStage::Transfer;
		fillBarrier.dstStageMask = PipelineStage::ComputeShader;
		fillBarrier.srcAccessMask = AccessFlags::TransferWrite;
		fillBarrier.dstAccessMask = AccessFlags::ShaderRead | AccessFlags::ShaderWrite;
		cmd->PipelineBarrier(fillBarrier);

		cmd->BindPipeline(*m_Pipeline);
		cmd->BindDescriptorSet(*m_PipelineLayout, *m_Context->sceneDataDescriptorSet[m_FrameIndex], 0, PipelineBindPoint::Compute);
		cmd->BindDescriptorSet(*m_PipelineLayout, *set, 1, PipelineBindPoint::Compute);

		CullPushConstants pc{};
		memcpy(pc.planes, m_Planes, sizeof(pc.planes));
		pc.objectCount = m_ObjectCount;
		pc.materialCount = m_MaterialCount;
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
}
