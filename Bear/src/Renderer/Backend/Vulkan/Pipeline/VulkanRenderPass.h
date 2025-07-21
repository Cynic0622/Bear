#pragma once
#include <vulkan/vulkan.h>

namespace Bear {

	class VulkanDevice;
	class VulkanSwapchain;

	class VulkanRenderPass {

	public:
		VulkanRenderPass(const VulkanDevice& device, const VulkanSwapchain& swapchain);
		~VulkanRenderPass();

		// ½ûÖ¹¿½±´ºÍÒÆ¶¯
		VulkanRenderPass(const VulkanRenderPass&) = delete;
		VulkanRenderPass& operator=(const VulkanRenderPass&) = delete;
		VulkanRenderPass(VulkanRenderPass&&) = delete;
		VulkanRenderPass& operator=(VulkanRenderPass&&) = delete;

		VkRenderPass GetHandle() const { return m_RenderPass; }

	private:
		const VulkanDevice& m_Device;
		const VulkanSwapchain& m_Swapchain;
		VkRenderPass m_RenderPass = VK_NULL_HANDLE;

	private:
		void CreateRenderPass();
	};
}