#pragma once
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "RHI/RHITypes.h"
#include "Pass.h"

namespace Bear
{
	class RenderContext;
	class RHIImage;
	class RHISampler;
	class RHIDescriptorSetLayout;
	class RHIDescriptorSet;
	class RHIPipeline;
	class RHIPipelineLayout;
	class RHICommandList;

	// Builds a max-depth Hi-Z pyramid (mip0 = half-res of the depth buffer) for occlusion culling.
	// Pyramids are ping-ponged per frame-in-flight: the culling pass reads the other slot
	// (previous frame's complete pyramid), the build writes the current slot.
	class HiZPass : public Pass
	{
	public:
		~HiZPass() override;

		void Setup(RenderContext* context) override;
		void Cleanup() override;
		const char* GetName() const override { return "HiZPass"; }
		void Resize();

		// rebuilds the pyramid for the given frame slot from the current depth buffer
		void Execute(RHICommandList* cmd, RHIImage& depthImage, uint32_t frameIndex);

		RHIImage* GetPyramid(uint32_t index) const { return m_Pyramids[index].get(); }
		RHISampler* GetSampler() const { return m_Sampler.get(); }
		uint32_t GetMipCount() const { return m_MipCount; }

	private:
		void CreatePyramids();
		void CreatePipeline();

		uint32_t m_MipCount = 1;
		uint32_t m_DepthWidth = 0;
		uint32_t m_DepthHeight = 0;

		std::shared_ptr<RHIImage> m_Pyramids[2];
		std::shared_ptr<RHISampler> m_Sampler;
		std::shared_ptr<RHIDescriptorSetLayout> m_DescriptorSetLayout;
		std::shared_ptr<RHIPipelineLayout> m_PipelineLayout;
		std::shared_ptr<RHIPipeline> m_Pipeline;
		// [frame-in-flight][mip level]; descriptors are only rewritten once per frame
		// (a set must not be updated while it is bound in the command buffer being recorded)
		std::vector<std::vector<std::unique_ptr<RHIDescriptorSet>>> m_DescriptorSets;
		uint32_t m_LastBuiltFrame = UINT32_MAX;
	};
}
