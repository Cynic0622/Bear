#pragma once
#include <vulkan/vulkan.h>
#include <vector>

namespace Bear {
	
	class VulkanDevice;
	class VulkanRenderPass;
	class VulkanFramebuffer {

	public:
		VulkanFramebuffer(const VulkanDevice& device, const VulkanRenderPass& renderPass, const std::vector<VkImageView>& attachments,
			uint32_t width, uint32_t height, uint32_t layers = 1);
		~VulkanFramebuffer();
		
		VulkanFramebuffer(const VulkanFramebuffer&) = delete;
		VulkanFramebuffer& operator=(const VulkanFramebuffer&) = delete;
		VulkanFramebuffer(VulkanFramebuffer&&) = delete;
		VulkanFramebuffer& operator=(VulkanFramebuffer&&) = delete;

		inline VkFramebuffer GetHandle() const { return m_Framebuffer; }

	private:
		const VulkanDevice& m_Device;
		const VulkanRenderPass& m_RenderPass;
		VkFramebuffer m_Framebuffer = VK_NULL_HANDLE;

	};
}