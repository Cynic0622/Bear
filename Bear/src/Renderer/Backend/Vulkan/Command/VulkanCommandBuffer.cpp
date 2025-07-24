#include "bearpch.h"

#include "VulkanCommandBuffer.h"
#include "VulkanCommandPool.h"
#include "Pipeline/VulkanRenderPass.h"
#include "Pipeline/VulkanPipeline.h"
#include "Pipeline/VulkanFramebuffer.h"
#include "VulkanDevice.h"

namespace Bear {

	VulkanCommandBuffer::VulkanCommandBuffer(const VulkanCommandPool& commandPool, const VkCommandBufferLevel& level, uint32_t commandBufferCount, const void* pNext)
		:m_Device(commandPool.GetDevice()), m_CommandPool(commandPool)
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_CommandPool.GetHandle();
		allocInfo.level = level;
		allocInfo.commandBufferCount = commandBufferCount;
		allocInfo.pNext = pNext;

		BEAR_CORE_ASSERT(vkAllocateCommandBuffers(m_Device.GetDevice(), &allocInfo, &m_CommandBuffer) == VK_SUCCESS, "Failed to allocate command buffers!");
	}
	VulkanCommandBuffer::~VulkanCommandBuffer()
	{
		m_CommandBuffer = VK_NULL_HANDLE;
		/*if (m_CommandBuffer != VK_NULL_HANDLE) {
			vkFreeCommandBuffers(m_Device.GetDevice(), m_CommandPool.GetHandle(), 1, &m_CommandBuffer);
			m_CommandBuffer = VK_NULL_HANDLE;
		}*/
		/*m_Device = nullptr;
		m_CommandPool = nullptr;*/
	}

	void VulkanCommandBuffer::Begin(VkCommandBufferUsageFlags flags, const void* pNext)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = flags;
		beginInfo.pNext = pNext;
		BEAR_CORE_ASSERT(vkBeginCommandBuffer(m_CommandBuffer, &beginInfo) == VK_SUCCESS, "Failed to begin command buffer!");
	}

	void VulkanCommandBuffer::End()
	{
		BEAR_CORE_ASSERT(vkEndCommandBuffer(m_CommandBuffer) == VK_SUCCESS, "Failed to end command buffer!");
	}

	void VulkanCommandBuffer::Reset(VkCommandBufferResetFlags flags)
	{
		BEAR_CORE_ASSERT(vkResetCommandBuffer(m_CommandBuffer, flags) == VK_SUCCESS, "Failed to reset command buffer!");
	}
	// 自定义清除值
	void VulkanCommandBuffer::BeginRenderPass(const VulkanRenderPass& renderPass, const VulkanFramebuffer& framebuffer, const VkExtent2D& swapchainExtent, const std::array<VkClearValue, 2>& clearValues, const VkSubpassContents& contents)
	{
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass.GetHandle();
		renderPassInfo.framebuffer = framebuffer.GetHandle();
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapchainExtent;
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();
		vkCmdBeginRenderPass(m_CommandBuffer, &renderPassInfo, contents);
	}
	void VulkanCommandBuffer::EndRenderPass()
	{
		vkCmdEndRenderPass(m_CommandBuffer);
	}
	void VulkanCommandBuffer::BindPipeline(const VulkanPipeline& pipeline)
	{
		BEAR_CORE_ASSERT(pipeline.GetHandle() != VK_NULL_HANDLE, "Pipeline handle is null!");
		vkCmdBindPipeline(m_CommandBuffer, pipeline.GetBindPoint(), pipeline.GetHandle());
	}
	// 使用默认清除值
	void VulkanCommandBuffer::BeginRenderPass(const VulkanRenderPass& renderPass, const VulkanFramebuffer& framebuffer, const VkExtent2D& swapchainExtent, const VkSubpassContents& contents)
	{
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass.GetHandle();
		renderPassInfo.framebuffer = framebuffer.GetHandle();
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapchainExtent;

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { {0.0f, 1.f, 0.0f, 1.0f} }; // 颜色附件的清除值
		clearValues[1].depthStencil = { 1.0f, 0 };             // 深度附件的清除值

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();
		vkCmdBeginRenderPass(m_CommandBuffer, &renderPassInfo, contents);
	}

	void VulkanCommandBuffer::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth) {
		VkViewport viewport{ x, y, width, height, minDepth, maxDepth };
		vkCmdSetViewport(m_CommandBuffer, 0, 1, &viewport);
	}

	void VulkanCommandBuffer::SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) {
		VkRect2D scissor{ {x, y}, {width, height} };
		vkCmdSetScissor(m_CommandBuffer, 0, 1, &scissor);
	}

	void VulkanCommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
		vkCmdDraw(m_CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
	}
}