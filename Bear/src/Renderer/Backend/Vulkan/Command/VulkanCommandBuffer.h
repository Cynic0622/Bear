#include "bearpch.h"

#include <vulkan/vulkan.h>

namespace Bear {

	class VulkanDevice;
	class VulkanCommandPool;
	class VulkanRenderPass;
	class VulkanPipeline;
	class VulkanFramebuffer;
	class VulkanCommandBuffer {
		public:
		VulkanCommandBuffer(const VulkanCommandPool& commandPool, const VkCommandBufferLevel& level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, uint32_t commandBufferCount = 1, const void* pNext = nullptr);
		~VulkanCommandBuffer();

		VulkanCommandBuffer(const VulkanCommandBuffer&) = delete;
		VulkanCommandBuffer& operator=(const VulkanCommandBuffer&) = delete;

		void Begin(VkCommandBufferUsageFlags flags = 0, const void* pNext = nullptr);
		void End();
		void Reset(VkCommandBufferResetFlags flags = 0);
		void BeginRenderPass(const VulkanRenderPass& renderPass, const VulkanFramebuffer& framebuffer, const VkExtent2D& swapchainExtent, const VkSubpassContents& contents = VK_SUBPASS_CONTENTS_INLINE);
		void BeginRenderPass(const VulkanRenderPass& renderPass, const VulkanFramebuffer& framebuffer, const VkExtent2D& swapchainExtent, const std::array<VkClearValue, 2>& clearValues, const VkSubpassContents& contents = VK_SUBPASS_CONTENTS_INLINE);
		void EndRenderPass();
		void BindPipeline(const VulkanPipeline& pipeline);
		void SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth);
		void SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height);
		void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);

		inline VkCommandBuffer GetHandle() const { return m_CommandBuffer; }

	private:
		const VulkanCommandPool& m_CommandPool;
		const VulkanDevice& m_Device;
		VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
	};
}