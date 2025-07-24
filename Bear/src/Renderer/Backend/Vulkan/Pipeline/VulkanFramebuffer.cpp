#include "bearpch.h"
#include "VulkanFramebuffer.h"
#include "Core/VulkanDevice.h"
#include "VulkanRenderPass.h"
namespace Bear {
	VulkanFramebuffer::VulkanFramebuffer(const VulkanDevice& device, const VulkanRenderPass& renderPass, 
		const std::vector<VkImageView>& attachments, uint32_t width, uint32_t height, uint32_t layers)
		:m_Device(device), m_RenderPass(renderPass)
	{
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass.GetHandle();
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = width;
		framebufferInfo.height = height;
		framebufferInfo.layers = layers;
		
		BEAR_CORE_ASSERT(vkCreateFramebuffer(device.GetDevice(), &framebufferInfo, nullptr, &m_Framebuffer) == VK_SUCCESS, "Failed to create framebuffer!");
	}
	VulkanFramebuffer::~VulkanFramebuffer()
	{
		if (m_Framebuffer != VK_NULL_HANDLE) {
			vkDestroyFramebuffer(m_Device.GetDevice(), m_Framebuffer, nullptr);
			m_Framebuffer = VK_NULL_HANDLE;
		}
	}
}