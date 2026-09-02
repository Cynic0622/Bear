#include "bearpch.h"

#include <vulkan/vulkan.h>
#include "RHI/RHICommandList.h"
#include "RHI/RHITypes.h"
#include "RHI/RHIPipeline.h"
namespace Bear {
	class RHIImage;

	class Device;
	class CommandPool;
	class RenderPass;
	class Pipeline;
	class Framebuffer;
	class Buffer;
	class CommandBuffer : public RHICommandList {
	public:
		CommandBuffer(const CommandPool& commandPool, const VkCommandBufferLevel& level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, uint32_t commandBufferCount = 1, const void* pNext = nullptr);
		~CommandBuffer() override;

		CommandBuffer(const CommandBuffer&) = delete;
		CommandBuffer& operator=(const CommandBuffer&) = delete;

		void Begin() override;
		void End() override;
		void Reset() override;
		void BeginRenderPass(RHIRenderPass& rhiRenderPass, RHIFramebuffer& rhiFramebuffer, uint32_t width, uint32_t height, const std::vector<RHIClearValue>& clearValues) override;
		
		void EndRenderPass() override;
		void BindPipeline(const RHIPipeline& pipeline) override;
		void SetViewport(float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f) override;
		void SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) override;
		void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;
		void BindVertexBuffer(const RHIBuffer& buffer, uint32_t binding = 0, size_t offset = 0) override; // rhi
		void BindIndexBuffer(const RHIBuffer& buffer, size_t offset = 0) override; // rhi
		void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0) override;
		void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;
		void BindDescriptorSet(const RHIPipelineLayout& pipelineLayout, const RHIDescriptorSet& set, uint32_t setIndex = 0, PipelineBindPoint bindPoint = PipelineBindPoint::Graphics) override;
		void TransitionImageLayout(RHIImage& texture, ImageLayout oldLayout, ImageLayout newLayout, uint32_t baseMipLevel = 0, uint32_t levelCount = 1) override;
		void CopyBufferToTexture(const RHIBuffer& srcBuffer, RHIImage& dstTexture) override;
		void PushConstants(const RHIPipelineLayout& pipelineLayout, ShaderStage stage, const void* data, size_t size, uint32_t offset) override;
		void NextSubpass() override;
		void* GetNativeHandle() const override { return m_CommandBuffer; }
		void CopyBuffer(const RHIBuffer& srcBuffer, const RHIBuffer& dstBuffer, size_t size, size_t srcOffset, size_t dstOffset) override;
		void PipelineBarrier(const MemoryBarrier& memoryBarrier) override;
		void FillBuffer(const RHIBuffer& buffer, const void* data, size_t size, size_t offset) override;
		void ClearImage(const RHIImage& image, const ClearColor& clearColor) override;
		void BlitImage(RHIImage& srcImage, RHIImage& dstImage, uint32_t srcLevel, uint32_t dstLevel) override;
		void DrawIndexedIndirect(const RHIBuffer& buffer, uint32_t drawCount, uint32_t stride, size_t offset) override;
		void DrawIndexedIndirectCount(const RHIBuffer& buffer, const RHIBuffer& countBuffer, uint32_t maxDrawCount, uint32_t stride, size_t offset, size_t countBufferOffset) override;
		VkCommandBuffer GetHandle() const { return m_CommandBuffer; }

	private:
		const CommandPool& m_CommandPool;
		const Device& m_Device;
		VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
	};
}