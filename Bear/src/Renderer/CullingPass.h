#pragma once
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "RHI/RHITypes.h"
#include "Pass.h"

namespace Bear
{
	class RenderContext;
	class RHIPipeline;
	class RHIPipelineLayout;
	class RHIDescriptorSetLayout;
	class RHIDescriptorSet;
	class RHIBuffer;
	class RHIImage;
	class RHISampler;
	class RHICommandList;
	struct RenderObject;
	class Mesh;
	class Material;

	// One contiguous run of commands sharing the same material in the input pool.
	struct CullingMaterialRange
	{
		uint32_t materialIndex; // index into region bases / counters
		uint32_t start;         // start offset in the input command pool
		uint32_t count;         // number of commands in this range
		std::shared_ptr<Material> material;
	};


	struct GpuCullStats
	{
		uint32_t visibleA = 0;   // phase 1 直接可见
		uint32_t rejected = 0;   // phase 1 遮挡可疑
		uint32_t rescued = 0;    // phase 2 救回
	};

	// A drawable indirect command set produced by the culling pass.
	// Consumers (PbrPass) just draw it; they don't care how it was produced.
	struct IndirectDrawSet
	{
		RHIBuffer* commands = nullptr;
		RHIBuffer* counters = nullptr;
		const std::vector<CullingMaterialRange>* ranges = nullptr;
		const std::vector<uint32_t>* regionBases = nullptr;
		uint32_t objectCount = 0; // CPU-side count, for stats only
	};

	// GPU occlusion + frustum culling.
	//
	// Always builds the CPU-side command pool and uploads it. When enabled, phase 1
	// also dispatches the GPU culler (frustum + previous-frame Hi-Z occlusion); when
	// occlusion is on, phase 2 retests deferred objects against the same-frame Hi-Z.
	class CullingPass : public Pass
	{
	public:
		void Setup(RenderContext* context) override;
		void Cleanup() override;
		const char* GetName() const override { return "CullingPass"; }

		void SetViewProjection(const glm::mat4& viewProjection);
		void SetOcclusionEnabled(bool enabled) { m_OcclusionEnabled = enabled; }
		bool IsOcclusionEnabled() const { return m_OcclusionEnabled; }
		bool IsOcclusionActive() const { return m_Enabled && m_OcclusionEnabled && m_HiZImage != nullptr; }
		void SetHiZSource(RHIImage* pyramid, RHISampler* sampler) { m_HiZImage = pyramid; m_HiZSampler = sampler; }
		void SetHiZMipCount(uint32_t mipCount) { m_HiZMipCount = mipCount; }

		void Execute(RHICommandList* cmd, const std::vector<RenderObject>& renderObjects);
		void ExecutePhase2(RHICommandList* cmd);

		// drawable command sets (valid after Execute / ExecutePhase2 for this frame)
		IndirectDrawSet GetVisibleSet() const { return { GetOutputCommandsBuffer(), GetCountersBuffer(), &m_MaterialRanges, &m_RegionBases, m_ObjectCount }; }
		IndirectDrawSet GetRescuedSet() const { return { GetRescuedCommandsBuffer(), GetRescuedCountersBuffer(), &m_MaterialRanges, &m_RegionBases, 0 }; }

		// results consumed by PbrPass (valid after Execute / ExecutePhase2)
		const std::vector<DrawIndexedIndirectCommand>& GetCommands() const { return m_Commands; }
		const std::vector<CullingMaterialRange>& GetMaterialRanges() const { return m_MaterialRanges; }
		const std::vector<uint32_t>& GetRegionBases() const { return m_RegionBases; }
		uint32_t GetObjectCount() const { return m_ObjectCount; }
		uint32_t GetMaterialCount() const { return m_MaterialCount; }
		RHIBuffer* GetOutputCommandsBuffer() const { return m_OutputCommandsBuffers[m_FrameIndex].get(); }   // OutA
		RHIBuffer* GetCountersBuffer() const { return m_CountersBuffers[m_FrameIndex].get(); }               // countersA
		RHIBuffer* GetRescuedCommandsBuffer() const { return m_RescuedCommandsBuffers[m_FrameIndex].get(); } // OutB
		RHIBuffer* GetRescuedCountersBuffer() const { return m_RescuedCountersBuffers[m_FrameIndex].get(); } // countersB
		const GpuCullStats& GetGpuStats() const { return m_GpuStats; }

	private:
		void CreatePipelines();
		void EnsureBuffers(uint32_t objectCount, uint32_t materialCount);
		void BuildCommandPool(const std::vector<RenderObject>& renderObjects);
		void UpdateAabbBuffer(const std::vector<RenderObject>& renderObjects);

		bool m_OcclusionEnabled = false;

		glm::vec4 m_Planes[6]{};
		uint32_t m_FrameIndex = 0;
		uint32_t m_ObjectCount = 0;
		uint32_t m_MaterialCount = 0;

		RHIImage* m_HiZImage = nullptr;
		RHISampler* m_HiZSampler = nullptr;
		uint32_t m_HiZMipCount = 1;

		std::shared_ptr<RHIDescriptorSetLayout> m_DescriptorSetLayout = nullptr;
		std::shared_ptr<RHIPipelineLayout> m_PipelineLayout = nullptr;
		std::shared_ptr<RHIPipeline> m_Pipeline = nullptr;
		std::shared_ptr<RHIDescriptorSetLayout> m_Phase2DescriptorSetLayout = nullptr;
		std::shared_ptr<RHIPipelineLayout> m_Phase2PipelineLayout = nullptr;
		std::shared_ptr<RHIPipeline> m_Phase2Pipeline = nullptr;

		std::vector<std::unique_ptr<RHIDescriptorSet>> m_DescriptorSets;       // phase 1, per frame
		std::vector<std::unique_ptr<RHIDescriptorSet>> m_Phase2DescriptorSets; // phase 2, per frame

		std::vector<std::unique_ptr<RHIBuffer>> m_AabbBuffers;
		std::vector<std::unique_ptr<RHIBuffer>> m_InputCommandsBuffers;
		std::vector<std::unique_ptr<RHIBuffer>> m_OutputCommandsBuffers;   // OutA
		std::vector<std::unique_ptr<RHIBuffer>> m_CountersBuffers;         // countersA (M+1)
		std::vector<std::unique_ptr<RHIBuffer>> m_MaterialIdBuffers;
		std::vector<std::unique_ptr<RHIBuffer>> m_RegionBaseBuffers;
		std::vector<std::unique_ptr<RHIBuffer>> m_RejectedBuffers;         // compact rejected commands (24B each)
		std::vector<std::unique_ptr<RHIBuffer>> m_RejectedCountersBuffers; // 1 uint
		std::vector<std::unique_ptr<RHIBuffer>> m_RescuedCommandsBuffers;  // OutB
		std::vector<std::unique_ptr<RHIBuffer>> m_RescuedCountersBuffers;  // countersB (M+1)
		std::vector<std::unique_ptr<RHIBuffer>> m_TotalsBuffers;           // [0]=A total, [1]=rejected, [2]=rescued

		GpuCullStats m_GpuStats;

		// CPU-side command pool, rebuilt every frame
		std::vector<DrawIndexedIndirectCommand> m_Commands;
		std::vector<CullingMaterialRange> m_MaterialRanges;
		std::vector<uint32_t> m_RegionBases;
		std::vector<uint32_t> m_MaterialIds;

		// AABB static reuse: AABBs are per-mesh, re-uploaded only when the mesh set changes
		struct AabbUpload { glm::vec4 min; glm::vec4 max; };
		std::vector<const Mesh*> m_CachedMeshes;
		std::vector<AabbUpload> m_AabbData;
	};
}
