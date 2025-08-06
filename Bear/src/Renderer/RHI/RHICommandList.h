#pragma once
#include <vector>
namespace Bear {

	class RHIPipeline;
	class RHIBuffer;
	struct RHIClearValue;
	class RHIRenderPass;
	class RHIPipelineLayout;
	class RHIDescriptorSet;
	class RHIFramebuffer;
	class RHITexture;
	enum class ImageLayout;
	class RHICommandList {
	public:
		virtual ~RHICommandList() = default;

		virtual void Begin() = 0;
		virtual void End() = 0;
		virtual void Reset() = 0;

		virtual void BeginRenderPass(RHIRenderPass& rhiRenderPass, RHIFramebuffer& rhiFramebuffer, uint32_t width, uint32_t height, const std::vector<RHIClearValue>& clearValues) = 0;
		virtual void EndRenderPass() = 0;

		virtual void BindPipeline(const RHIPipeline& pipeline) = 0;
		virtual void BindDescriptorSet(const RHIPipelineLayout& pipelineLayout, const RHIDescriptorSet& set, uint32_t setIndex = 0) = 0;
		virtual void BindVertexBuffer(const RHIBuffer& buffer, uint32_t binding = 0, size_t offset = 0) = 0;
		virtual void BindIndexBuffer(const RHIBuffer& buffer, size_t offset = 0) = 0;

		// йс©з╪Т╡ц
		virtual void SetViewport(float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f) = 0;
		virtual void SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) = 0;

		virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;
		virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0) = 0;

		virtual void TransitionImageLayout(RHITexture& texture, ImageLayout oldLayout, ImageLayout newLayout) = 0;
		virtual void CopyBufferToTexture(const RHIBuffer& srcBuffer, RHITexture& dstTexture) = 0;
	};
}