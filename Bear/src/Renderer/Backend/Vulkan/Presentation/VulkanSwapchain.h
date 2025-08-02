#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include "RHI/RHISwapchain.h"
#include "RHI/RHIFramebuffer.h"
#include "Pipeline/VulkanFrameBuffer.h"

namespace Bear {

	class VulkanDevice;
	class VulkanSurface;
	class VulkanRenderPass;
	class VulkanFramebuffer;
	class VulkanImage;
	class VulkanSemaphore;

	class VulkanSwapchain : public RHISwapchain {
		public:
		VulkanSwapchain(VulkanDevice& device, VulkanRenderPass& renderPass);
		~VulkanSwapchain();

		// 禁止拷贝和移动
		VulkanSwapchain(const VulkanSwapchain&) = delete;
		VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;
		VulkanSwapchain(VulkanSwapchain&&) = delete;
		VulkanSwapchain& operator=(VulkanSwapchain&&) = delete;

		
		RHIFramebuffer* GetFramebuffer(uint32_t index) const override { return m_Framebuffers[index].get(); }

		VkResult AcquireNextImage(uint32_t* imageIndex, VkSemaphore semaphore);
		uint32_t AcquireNextImage(VulkanSemaphore* imageAvailableSemaphore); // rhi
		void Present(uint32_t imageIndex, VulkanSemaphore* renderFinishedSemaphore); // rhi
		void Resize(); // rhi

		uint32_t GetWidth() const override { return m_Extent.width; } // rhi
		uint32_t GetHeight() const override { return m_Extent.height; } // rhi

		VkSwapchainKHR GetHandle() const { return m_Swapchain; }
		VkFormat GetImageFormat() const { return m_ImageFormat; }
		VkExtent2D GetExtent() const { return m_Extent; }
		const std::vector<VkImageView>& GetImageViews() const { return m_ImageViews; }
		uint32_t GetImageCount() const override { return static_cast<uint32_t>(m_Images.size()); } //rhi

	private:
		void Init();
		void Cleanup();
		void Recreate();
		void CleanupFramebuffers();

		// 初始化过程中的详细步骤
		void ChooseSurfaceFormat();
		void ChoosePresentMode();
		void ChooseExtent();


		void CreateSwapchain();
		void CreateImageViews();
		void CreateFramebuffers(const VulkanRenderPass& renderPass);
		void CreateDepthResources();

		VkResult SubmitImage(uint32_t imageIndex, VkQueue presentQueue, VkSemaphore waitSemaphore);

	private:
		const VulkanDevice& m_Device; // 持有对设备的引用
		const VulkanRenderPass& m_RenderPass;

		VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;

		// 交换链资源
		std::vector<VkImage> m_Images;
		std::vector<VkImageView> m_ImageViews;
		VkFormat m_ImageFormat;
		VkExtent2D m_Extent;
		uint32_t m_ImageCount = 0; // 图像数量
		uint32_t m_MinImageCount = 2; // 最小图像数量

		// 协商后选择的参数
		VkSurfaceFormatKHR m_SurfaceFormat;
		VkPresentModeKHR m_PresentMode;

		std::vector<std::unique_ptr<VulkanFramebuffer>> m_Framebuffers; // 用于渲染的帧缓冲区
		std::unique_ptr<VulkanImage> m_DepthImage;
	};
}