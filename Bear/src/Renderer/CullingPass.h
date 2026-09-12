#pragma once
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "RHI/RHITypes.h"

namespace Bear
{
	class RenderContext;
	class RHIPipeline;
	class RHIPipelineLayout;
	class RHIDescriptorSetLayout;
	class RHIDescriptorSet;
	class RHIBuffer;
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

	// GPU frustum culling pass.
	// Every frame it builds the input command pool (opaque objects sorted by material),
	// uploads per-object AABBs (only when the mesh set changes), dispatches a compute
	// shader that frustum-culls each object and compacts surviving commands into
	// per-material buckets with per-material atomic counters.
	class CullingPass
	{
	public:
		void Setup(RenderContext* context);
		void Cleanup();

		void SetViewProjection(const glm::mat4& viewProjection);
		void SetEnabled(bool enabled) { m_Enabled = enabled; }
		bool IsEnabled() const { return m_Enabled; }

		void Execute(RHICommandList* cmd, const std::vector<RenderObject>& renderObjects);

		// results consumed by PbrPass (valid after Execute, same frame index)
		const std::vector<DrawIndexedIndirectCommand>& GetCommands() const { return m_Commands; }
		const std::vector<CullingMaterialRange>& GetMaterialRanges() const { return m_MaterialRanges; }
		const std::vector<uint32_t>& GetRegionBases() const { return m_RegionBases; }
		uint32_t GetObjectCount() const { return m_ObjectCount; }
		uint32_t GetMaterialCount() const { return m_MaterialCount; }
		RHIBuffer* GetOutputCommandsBuffer() const;
		RHIBuffer* GetCountersBuffer() const;

	private:
		void CreatePipeline();
		void EnsureBuffers(uint32_t objectCount, uint32_t materialCount);
		void BuildCommandPool(const std::vector<RenderObject>& renderObjects);
		void UpdateAabbBuffer(const std::vector<RenderObject>& renderObjects);

		RenderContext* m_Context = nullptr;
		bool m_Enabled = false;

		glm::vec4 m_Planes[6]{};
		uint32_t m_FrameIndex = 0;
		uint32_t m_ObjectCount = 0;
		uint32_t m_MaterialCount = 0;

		std::shared_ptr<RHIDescriptorSetLayout> m_DescriptorSetLayout = nullptr;
		std::shared_ptr<RHIPipelineLayout> m_PipelineLayout = nullptr;
		std::shared_ptr<RHIPipeline> m_Pipeline = nullptr;

		std::vector<std::unique_ptr<RHIDescriptorSet>> m_DescriptorSets;
		std::vector<std::unique_ptr<RHIBuffer>> m_AabbBuffers;
		std::vector<std::unique_ptr<RHIBuffer>> m_InputCommandsBuffers;
		std::vector<std::unique_ptr<RHIBuffer>> m_OutputCommandsBuffers;
		std::vector<std::unique_ptr<RHIBuffer>> m_CountersBuffers;
		std::vector<std::unique_ptr<RHIBuffer>> m_MaterialIdBuffers;
		std::vector<std::unique_ptr<RHIBuffer>> m_RegionBaseBuffers;

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
