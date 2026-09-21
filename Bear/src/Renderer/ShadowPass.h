#pragma once
#include "Pass.h"
#include "BaseData.h"
#include <array>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

namespace Bear
{
	class CullingPass;
	class RHICommandList;
	class RHIPipeline;
	class RHIPipelineLayout;
	class RHIDescriptorSetLayout;
	class RHIDescriptorSet;
	class RHIBuffer;
	class RHIImage;
	class RHISampler;

	// Cascaded shadow maps (4 cascades, 2D depth array) with GPU per-cascade culling.
	// One graph pass per cascade: cull against the light frustum, then render depth into
	// the corresponding array layer.
	class ShadowPass : public Pass
	{
	public:
		static constexpr uint32_t kCascadeCount = 4;
		static constexpr uint32_t kShadowResolution = 2048;

		void Setup(RenderContext* context) override;
		void Cleanup() override;
		const char* GetName() const override { return "ShadowPass"; }

		// computes the cascade light matrices for this frame and fills the SceneData fields
		void UpdateCascades(const glm::mat4& view, const glm::mat4& proj, float nearPlane, float farPlane,
			const glm::vec3& lightDirection, const glm::vec3& casterBoundsMin, const glm::vec3& casterBoundsMax,
			SceneData& sceneData);

		// executes cascade: GPU cull (using CullingPass input pools) + depth-only render
		void Execute(RHICommandList* cmd, uint32_t cascadeIndex, CullingPass& culling, RHIImage* shadowMap);

		RHITextureConfig GetShadowMapConfig() const;
		RHISampler* GetSampler() const { return m_Sampler.get(); }
		// caster counts read back from the previous frame (diagnostics)
		const std::array<uint32_t, kCascadeCount>& GetVisibleCounts() const { return m_LastVisibleCounts; }

	private:
		void CreatePipelines();
		// per frame-in-flight: returns true when this slot's command buffers were (re)created
		// and the slot's descriptor sets must be rewritten
		bool EnsureBuffers(uint32_t frameIndex, uint32_t objectCount);
		void UpdateDescriptorSets(uint32_t frameIndex, CullingPass& culling);

		RenderContext* m_Context = nullptr;

		std::array<glm::mat4, kCascadeCount> m_LightViewProj{};
		std::array<float, kCascadeCount> m_Splits{};
		std::array<glm::vec4, kCascadeCount * 6> m_Planes{}; // per-cascade light frustum planes
		std::array<float, kCascadeCount> m_TexelWorldSize{};
		// per-cascade near-plane extension toward the light; cached so that the depth range only
		// grows when the caster bounds require it (grow at once, shrink with a hysteresis step)
		std::array<float, kCascadeCount> m_NearExtend{};

		std::shared_ptr<RHISampler> m_Sampler;
		std::shared_ptr<RHIDescriptorSetLayout> m_CullSetLayout;
		std::shared_ptr<RHIPipelineLayout> m_CullPipelineLayout;
		std::shared_ptr<RHIPipeline> m_CullPipeline;
		std::shared_ptr<RHIPipelineLayout> m_RasterPipelineLayout;
		std::shared_ptr<RHIPipeline> m_RasterPipeline;

		// per frame-in-flight, per cascade
		std::vector<std::array<std::unique_ptr<RHIBuffer>, kCascadeCount>> m_CommandBuffers;
		std::vector<std::array<std::unique_ptr<RHIBuffer>, kCascadeCount>> m_CounterBuffers;
		std::vector<std::array<std::unique_ptr<RHIDescriptorSet>, kCascadeCount>> m_CullSets;
		std::array<uint32_t, kCascadeCount> m_LastVisibleCounts{};
	};
}
