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
		void BindVertexBuffer(const RHIBuffer& buffer, uint32_t binding = 0, size_t offset = 0) override; // ʵ��RHI�ӿ�
		void BindIndexBuffer(const RHIBuffer& buffer, size_t offset = 0) override; // ʵ��RHI�ӿ�
		void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0) override;
		void BindDescriptorSet(const RHIPipelineLayout& pipelineLayout, const RHIDescriptorSet& set, uint32_t setIndex = 0) override;
		void TransitionImageLayout(RHIImage& texture, ImageLayout oldLayout, ImageLayout newLayout) override;
		void CopyBufferToTexture(const RHIBuffer& srcBuffer, RHIImage& dstTexture) override;
		inline VkCommandBuffer GetHandle() const { return m_CommandBuffer; }

	private:
		const CommandPool& m_CommandPool;
		const Device& m_Device;
		VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
	};
}