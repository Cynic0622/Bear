#include "bearpch.h"
#include "Framebuffer.h"
#include "Core/Device.h"
#include "RenderPass.h"
namespace Bear {
	Framebuffer::Framebuffer(const Device& device, const RenderPass& renderPass, 
		const std::vector<VkImageView>& attachments, uint32_t width, uint32_t height, uint32_t layers)
		:m_Device(device)
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

	Framebuffer::Framebuffer(const Device& device, VkFramebufferCreateInfo& createInfo)
		:m_Device(device)
	{
		BEAR_CORE_ASSERT(vkCreateFramebuffer(device.GetDevice(), &createInfo, nullptr, &m_Framebuffer) == VK_SUCCESS, "Failed to create framebuffer!");
	}

	Framebuffer::~Framebuffer()
	{
		if (m_Framebuffer != VK_NULL_HANDLE) {
			vkDestroyFramebuffer(m_Device.GetDevice(), m_Framebuffer, nullptr);
			m_Framebuffer = VK_NULL_HANDLE;
		}
	}
}