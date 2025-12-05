#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "RHI/RHIFramebuffer.h"
namespace Bear {
	
	class Device;
	class RenderPass;
	class Framebuffer : public RHIFramebuffer {

	public:
		Framebuffer(const Device& device, const RenderPass& renderPass, const std::vector<VkImageView>& attachments,
			uint32_t width, uint32_t height, uint32_t layers = 1);
		Framebuffer(const Device& device, VkFramebufferCreateInfo& createInfo);
		~Framebuffer() override;
		
		Framebuffer(const Framebuffer&) = delete;
		Framebuffer& operator=(const Framebuffer&) = delete;
		Framebuffer(Framebuffer&&) = delete;
		Framebuffer& operator=(Framebuffer&&) = delete;

		inline VkFramebuffer GetHandle() const { return m_Framebuffer; }

	private:
		const Device& m_Device;
		// const RenderPass& m_RenderPass;
		VkFramebuffer m_Framebuffer = VK_NULL_HANDLE;

	};
}