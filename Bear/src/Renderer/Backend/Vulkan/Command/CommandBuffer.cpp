#include "bearpch.h"

#include "CommandBuffer.h"
#include "CommandPool.h"
#include "Pipeline/RenderPass.h"
#include "Pipeline/Pipeline.h"
#include "Pipeline/Framebuffer.h"
#include "Core/Device.h"
#include "Resources/Buffer.h"
#include "Pipeline/PipelineLayout.h"
#include "Pipeline/DescriptorSet.h"
namespace Bear {

	CommandBuffer::CommandBuffer(const CommandPool& commandPool, const VkCommandBufferLevel& level, uint32_t commandBufferCount, const void* pNext)
		:m_Device(commandPool.GetDevice()), m_CommandPool(commandPool)
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_CommandPool.GetHandle();
		allocInfo.level = level;
		allocInfo.commandBufferCount = commandBufferCount;
		allocInfo.pNext = pNext;

		BEAR_CORE_ASSERT(vkAllocateCommandBuffers(m_Device.GetDevice(), &allocInfo, &m_CommandBuffer) == VK_SUCCESS, "Failed to allocate command buffers!")
	}
	CommandBuffer::~CommandBuffer()
	{
		m_CommandBuffer = VK_NULL_HANDLE;
		/*if (m_CommandBuffer != VK_NULL_HANDLE) {
			vkFreeCommandBuffers(m_Device.GetDevice(), m_CommandPool.GetHandle(), 1, &m_CommandBuffer);
			m_CommandBuffer = VK_NULL_HANDLE;
		}*/
		/*m_Device = nullptr;
		m_CommandPool = nullptr;*/
	}

	void CommandBuffer::Begin()
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		BEAR_CORE_ASSERT(vkBeginCommandBuffer(m_CommandBuffer, &beginInfo) == VK_SUCCESS, "Failed to begin command buffer!")
	}

	void CommandBuffer::End()
	{
		BEAR_CORE_ASSERT(vkEndCommandBuffer(m_CommandBuffer) == VK_SUCCESS, "Failed to end command buffer!")
	}

	void CommandBuffer::Reset()
	{
		BEAR_CORE_ASSERT(vkResetCommandBuffer(m_CommandBuffer, 0) == VK_SUCCESS, "Failed to reset command buffer!")
	}
	
	void CommandBuffer::EndRenderPass()
	{
		vkCmdEndRenderPass(m_CommandBuffer);
	}
	void CommandBuffer::BindPipeline(const RHIPipeline& pipeline)
	{
		const auto& vkPipeline = dynamic_cast<const Pipeline&>(pipeline);
		BEAR_CORE_ASSERT(vkPipeline.GetHandle() != VK_NULL_HANDLE, "Pipeline handle is null!")
		vkCmdBindPipeline(m_CommandBuffer, vkPipeline.GetBindPoint(), vkPipeline.GetHandle());
	}

	void CommandBuffer::BeginRenderPass(RHIRenderPass& rhiRenderPass, RHIFramebuffer& rhiFramebuffer, uint32_t width, uint32_t height, const std::vector<RHIClearValue>& clearValues)
	{
		auto& renderPass = dynamic_cast<RenderPass&>(rhiRenderPass);
		auto& framebuffer = dynamic_cast<Framebuffer&>(rhiFramebuffer);
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass.GetHandle();
		renderPassInfo.framebuffer = framebuffer.GetHandle();
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

	void CommandBuffer::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth) {
		VkViewport viewport{ x, y, width, height, minDepth, maxDepth };
		vkCmdSetViewport(m_CommandBuffer, 0, 1, &viewport);
	}

	void CommandBuffer::SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) {
		VkRect2D scissor{ {x, y}, {width, height} };
		vkCmdSetScissor(m_CommandBuffer, 0, 1, &scissor);
	}

	void CommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
		vkCmdDraw(m_CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
	}
	void CommandBuffer::BindVertexBuffer(const RHIBuffer& buffer, uint32_t binding, size_t offset)
	{
		VkBuffer buffers[] = { dynamic_cast<const Buffer&>(buffer).GetHandle() };
		VkDeviceSize offsets[] = { static_cast<VkDeviceSize>(offset) };
		vkCmdBindVertexBuffers(m_CommandBuffer, binding, 1, buffers, offsets);
	}
	void CommandBuffer::BindIndexBuffer(const RHIBuffer& buffer, size_t offset)
	{
		const auto& vkBuffer = dynamic_cast<const Buffer&>(buffer);
		VkDeviceSize vkOffset = static_cast<VkDeviceSize>(offset);
		vkCmdBindIndexBuffer(m_CommandBuffer, vkBuffer.GetHandle(), vkOffset, VK_INDEX_TYPE_UINT16);
	}
	void CommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
	{
		vkCmdDrawIndexed(m_CommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}
	void CommandBuffer::BindDescriptorSet(const RHIPipelineLayout& pipelineLayout, const RHIDescriptorSet& descriptorSet, uint32_t firstSet)
	{
		const auto& vkPipelineLayout = dynamic_cast<const PipelineLayout&>(pipelineLayout);
		const auto& vkDescriptorSet = dynamic_cast<const DescriptorSet&>(descriptorSet);
		VkPipelineLayout pipelineLayoutHandle = vkPipelineLayout.GetHandle();
		VkDescriptorSet descriptorSetHandle = vkDescriptorSet.GetHandle();
		vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayoutHandle, firstSet, 1, &descriptorSetHandle, 0, nullptr);
	}
}