#pragma once
#include <vector>
#include "RHITypes.h"
namespace Bear {

	class RHIPipeline;
	class RHIBuffer;
	struct RHIClearValue;
	class RHIRenderPass;
	class RHIPipelineLayout;
	class RHIDescriptorSet;
	class RHIFramebuffer;
	class RHIImage;
	enum class ImageLayout;
	enum class AttachmentLoadOp;
	enum class AttachmentStoreOp;
	struct RHIClearValue;

	// Attachment description for dynamic rendering (vkCmdBeginRendering path).
	struct RHIRenderingAttachment
	{
		RHIImage* image = nullptr;
		ImageLayout layout = ImageLayout::ColorAttachment;
		AttachmentLoadOp loadOp = AttachmentLoadOp::Load;
		AttachmentStoreOp storeOp = AttachmentStoreOp::Store;
		RHIClearValue* clearValue = nullptr; // required when loadOp == Clear
		bool isDepth = false;
	};
	class RHICommandList {
	public:
		virtual ~RHICommandList() = default;

		virtual void Begin() = 0;
		virtual void End() = 0;
		virtual void Reset() = 0;

		virtual void BeginRenderPass(RHIRenderPass& rhiRenderPass, RHIFramebuffer& rhiFramebuffer, uint32_t width, uint32_t height, const std::vector<RHIClearValue>& clearValues) = 0;
		virtual void EndRenderPass() = 0;

		// dynamic rendering (vkCmdBeginRendering); layouts must already be transitioned by the caller
		virtual void BeginRendering(const std::vector<RHIRenderingAttachment>& attachments) = 0;
		virtual void EndRendering() = 0;

		virtual void BindPipeline(const RHIPipeline& pipeline) = 0;
		virtual void BindDescriptorSet(const RHIPipelineLayout& pipelineLayout, const RHIDescriptorSet& set, uint32_t setIndex = 0, PipelineBindPoint bindPoint = PipelineBindPoint::Graphics) = 0;
		virtual void BindVertexBuffer(const RHIBuffer& buffer, uint32_t binding = 0, size_t offset = 0) = 0;
		virtual void BindIndexBuffer(const RHIBuffer& buffer, size_t offset = 0) = 0;

		// �ӿڼ���
		virtual void SetViewport(float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f) = 0;
		virtual void SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) = 0;

		virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;
		virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0) = 0;
		virtual void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;

		virtual void DrawIndexedIndirect(const RHIBuffer& buffer, uint32_t drawCount, uint32_t stride, size_t offset = 0) = 0;
		virtual void DrawIndexedIndirectCount(const RHIBuffer& buffer, const RHIBuffer& countBuffer, uint32_t maxDrawCount, uint32_t stride, size_t offset = 0, size_t countBufferOffset = 0) = 0;

		virtual void TransitionImageLayout(RHIImage& texture, ImageLayout oldLayout, ImageLayout newLayout, uint32_t baseMipLevel = 0, uint32_t levelCount = 1) = 0;
		virtual void CopyBufferToTexture(const RHIBuffer& srcBuffer, RHIImage& dstTexture) = 0;

		// pipeline barrrier
		virtual void PipelineBarrier(const MemoryBarrier& memoryBarrier) = 0;
		virtual void ImageBarrier(RHIImage& image, const ImageBarrierDesc& barrier) = 0;

		virtual void BlitImage(RHIImage& srcImage, RHIImage& dstImage, uint32_t srcLevel, uint32_t dstLevel) = 0;

		// copy buffer
		virtual void CopyBuffer(const RHIBuffer& srcBuffer, const RHIBuffer& dstBuffer, size_t size, size_t srcOffset = 0, size_t dstOffset = 0) = 0;
		// fill buffer
		virtual void FillBuffer(const RHIBuffer& buffer, const void* data, size_t size, size_t offset = 0) = 0;
		// clear image color
		virtual void ClearImage(const RHIImage& image, const ClearColor& clearColor) = 0;

		// push constants
		virtual void PushConstants(const RHIPipelineLayout& pipelineLayout, ShaderStage stage, const void* data, size_t size, uint32_t offset) = 0;

		// next subpass
		virtual void NextSubpass() = 0;

		// get original handle
		virtual void* GetNativeHandle() const = 0;
	};
}