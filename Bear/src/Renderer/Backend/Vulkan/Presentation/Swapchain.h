#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include "RHI/RHISwapchain.h"
#include "RHI/RHIFramebuffer.h"
#include "Pipeline/Framebuffer.h"
#include "Resources/Image.h"
namespace Bear {

	class Device;
	class Surface;
	class RenderPass;
	class Framebuffer;
	class Image;
	class Semaphore;

	class Swapchain : public RHISwapchain {
		public:
		Swapchain(Device& device, RenderPass& renderPass);
		~Swapchain() override;

		// 禁止拷贝和移动
		Swapchain(const Swapchain&) = delete;
		Swapchain& operator=(const Swapchain&) = delete;
		Swapchain(Swapchain&&) = delete;
		Swapchain& operator=(Swapchain&&) = delete;

		
		RHIFramebuffer* GetFramebuffer(uint32_t index) const override { return m_Framebuffers[index].get(); } // rhi

		uint32_t AcquireNextImage(Semaphore& imageAvailableSemaphore);
		uint32_t AcquireNextImage() override; // rhi
		void Present(uint32_t imageIndex) override; // rhi
		void Present(uint32_t imageIndex, Semaphore& renderFinishedSemaphore); // rhi
		void Resize() override; // rhi

		uint32_t GetWidth() const override { return m_Extent.width; } // rhi
		uint32_t GetHeight() const override { return m_Extent.height; } // rhi

		VkSwapchainKHR GetHandle() const { return m_Swapchain; }
		VkFormat GetImageFormat() const { return m_ImageFormat; }
		void GetExtent(uint32_t& width, uint32_t& height) const override;
		const std::vector<VkImageView>& GetImageViews() const { return m_ImageViews; }
		void* GetDepthView() const override { return m_DepthImage->GetView(); } // rhi
		void* GetColorView(uint8_t index) const override { return m_ImageViews[index]; } // rhi
		uint32_t GetImageCount() const override { return static_cast<uint32_t>(m_Images.size()); } //rhi
		// const VkImageView& GetImage(uint32_t index) const { return m_ImageViews[index]; }
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
		void CreateFramebuffers(const RenderPass& renderPass);
		void CreateDepthResources();

		VkResult SubmitImage(uint32_t imageIndex, VkQueue presentQueue, VkSemaphore waitSemaphore);

	private:
		const Device& m_Device; // 持有对设备的引用
		const RenderPass& m_RenderPass;

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

		std::vector<std::unique_ptr<Framebuffer>> m_Framebuffers; // 用于渲染的帧缓冲区
		std::unique_ptr<Image> m_DepthImage;
	};
}