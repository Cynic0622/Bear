#include "bearpch.h"

#include "VulkanCommandBuffer.h"
#include "VulkanCommandPool.h"
#include "Pipeline/VulkanRenderPass.h"
#include "Pipeline/VulkanPipeline.h"
#include "Pipeline/VulkanFramebuffer.h"
#include "Core/VulkanDevice.h"
#include "Resources/VulkanBuffer.h"
#include "Pipeline/VulkanPipelineLayout.h"
#include "Pipeline/VulkanDescriptorSet.h"
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

	void VulkanCommandBuffer::Begin()
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		BEAR_CORE_ASSERT(vkBeginCommandBuffer(m_CommandBuffer, &beginInfo) == VK_SUCCESS, "Failed to begin command buffer!");
	}

	void VulkanCommandBuffer::End()
	{
		BEAR_CORE_ASSERT(vkEndCommandBuffer(m_CommandBuffer) == VK_SUCCESS, "Failed to end command buffer!");
	}

	void VulkanCommandBuffer::Reset()
	{
		BEAR_CORE_ASSERT(vkResetCommandBuffer(m_CommandBuffer, 0) == VK_SUCCESS, "Failed to reset command buffer!");
	}
	
	void VulkanCommandBuffer::EndRenderPass()
	{
		vkCmdEndRenderPass(m_CommandBuffer);
	}
	void VulkanCommandBuffer::BindPipeline(const RHIPipeline& pipeline)
	{
		const auto& vkPipeline = static_cast<const VulkanPipeline&>(pipeline);
		BEAR_CORE_ASSERT(vkPipeline.GetHandle() != VK_NULL_HANDLE, "Pipeline handle is null!");
		vkCmdBindPipeline(m_CommandBuffer, vkPipeline.GetBindPoint(), vkPipeline.GetHandle());
	}

	void VulkanCommandBuffer::BeginRenderPass(RHIRenderPass* rhiRenderPass, void* rhiFramebuffer, uint32_t width, uint32_t height, const std::vector<RHIClearValue>& clearValues)
	{
		auto renderPass = static_cast<VulkanRenderPass*>(rhiRenderPass);
		auto framebuffer = static_cast<VulkanFramebuffer*>(rhiFramebuffer);
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass->GetHandle();
		renderPassInfo.framebuffer = framebuffer->GetHandle();
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = { width, height };

		std::vector<VkClearValue> vkClearValues;
		vkClearValues.reserve(clearValues.size());

		for (const auto& clearValue : clearValues) {
			VkClearValue vkClearValue{};
			if (clearValue.isDepth)
				vkClearValue.depthStencil = { clearValue.depthStencil.depth, clearValue.depthStencil.stencil };
			else {
				vkClearValue.color = { { clearValue.color.r, clearValue.color.g, clearValue.color.b, clearValue.color.a } };
			}
			vkClearValues.push_back(vkClearValue);
		}

		renderPassInfo.clearValueCount = static_cast<uint32_t>(vkClearValues.size());
		renderPassInfo.pClearValues = vkClearValues.data();
		vkCmdBeginRenderPass(m_CommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
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
	void VulkanCommandBuffer::BindVertexBuffer(const RHIBuffer& buffer, uint32_t binding, size_t offset)
	{
		VkBuffer buffers[] = { static_cast<const VulkanBuffer&>(buffer).GetHandle() };
		VkDeviceSize offsets[] = { static_cast<VkDeviceSize>(offset) };
		vkCmdBindVertexBuffers(m_CommandBuffer, binding, 1, buffers, offsets);
	}
	void VulkanCommandBuffer::BindIndexBuffer(const RHIBuffer& buffer, size_t offset)
	{
		const auto& vkBuffer = static_cast<const VulkanBuffer&>(buffer);
		VkDeviceSize vkOffset = static_cast<VkDeviceSize>(offset);
		vkCmdBindIndexBuffer(m_CommandBuffer, vkBuffer.GetHandle(), vkOffset, VK_INDEX_TYPE_UINT16);
	}
	void VulkanCommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
	{
		vkCmdDrawIndexed(m_CommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}
	void VulkanCommandBuffer::BindDescriptorSet(const RHIPipelineLayout& pipelineLayout, const RHIDescriptorSet& descriptorSet, uint32_t firstSet)
	{
		const auto& vkPipelineLayout = static_cast<const VulkanPipelineLayout&>(pipelineLayout);
		const auto& vkDescriptorSet = static_cast<const VulkanDescriptorSet&>(descriptorSet);
		VkPipelineLayout pipelineLayoutHandle = vkPipelineLayout.GetHandle();
		VkDescriptorSet descriptorSetHandle = vkDescriptorSet.GetHandle();
		vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayoutHandle, firstSet, 1, &descriptorSetHandle, 0, nullptr);
	}
}