#include "bearpch.h"

#include "CommandBuffer.h"
#include "CommandPool.h"
#include "Image.h"
#include "Utils.h"
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

		BEAR_CORE_ASSERT(vkAllocateCommandBuffers(m_Device.GetDevice(), &allocInfo, &m_CommandBuffer) == VK_SUCCESS, "Failed to allocate command buffers!");
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
		BEAR_CORE_ASSERT(vkBeginCommandBuffer(m_CommandBuffer, &beginInfo) == VK_SUCCESS, "Failed to begin command buffer!");
	}

	void CommandBuffer::End()
	{
		BEAR_CORE_ASSERT(vkEndCommandBuffer(m_CommandBuffer) == VK_SUCCESS, "Failed to end command buffer!");
	}

	void CommandBuffer::Reset()
	{
		BEAR_CORE_ASSERT(vkResetCommandBuffer(m_CommandBuffer, 0) == VK_SUCCESS, "Failed to reset command buffer!");
	}
	
	void CommandBuffer::EndRenderPass()
	{
		vkCmdEndRenderPass(m_CommandBuffer);
	}
	void CommandBuffer::BindPipeline(const RHIPipeline& pipeline)
	{
		const auto& vkPipeline = dynamic_cast<const Pipeline&>(pipeline);
		BEAR_CORE_ASSERT(vkPipeline.GetHandle() != VK_NULL_HANDLE, "Pipeline handle is null!");
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
		vkCmdBindIndexBuffer(m_CommandBuffer, vkBuffer.GetHandle(), vkOffset, VK_INDEX_TYPE_UINT32);
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
	void CommandBuffer::TransitionImageLayout(RHIImage& texture, ImageLayout oldLayout, ImageLayout newLayout)
	{
		auto& vkImage = dynamic_cast<Image&>(texture);
		VkImageLayout vkOldLayout = ToVulkanImageLayout(oldLayout);
		VkImageLayout vkNewLayout = ToVulkanImageLayout(newLayout);
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = ToVulkanImageLayout(oldLayout);
		barrier.newLayout = ToVulkanImageLayout(newLayout);
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = vkImage.GetImage();
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (vkOldLayout == VK_IMAGE_LAYOUT_UNDEFINED && vkNewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (vkOldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && vkNewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else {
			throw std::invalid_argument("unsupported layout transition!");
		}

		vkCmdPipelineBarrier(m_CommandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}
	void CommandBuffer::CopyBufferToTexture(const RHIBuffer& srcBuffer, RHIImage& dstTexture)
	{
		const auto& vkBuffer = dynamic_cast<const Buffer&>(srcBuffer);
		auto& vkTexture = dynamic_cast<Image&>(dstTexture);

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { vkTexture.GetWidth(), vkTexture.GetHeight(), 1 };

		vkCmdCopyBufferToImage(m_CommandBuffer, vkBuffer.GetHandle(), vkTexture.GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	}
	void CommandBuffer::PushConstants(const RHIPipelineLayout& pipelineLayout, ShaderStage stage, const void* data, size_t size, uint32_t offset)
	{
		const auto& vkPipelineLayout = dynamic_cast<const PipelineLayout&>(pipelineLayout);
		vkCmdPushConstants(m_CommandBuffer, vkPipelineLayout.GetHandle(), ToVulkanShaderStage(stage), offset, size, data);
	}
}