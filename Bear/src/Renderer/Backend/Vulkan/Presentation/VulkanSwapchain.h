#pragma once

#include <vulkan/vulkan.h>
#include <vector>
namespace Bear {

	class VulkanDevice;
	class VulkanSurface;
	// Forward declaration of Window class
	class Window;

	class VulkanSwapchain {
		public:
		VulkanSwapchain(VulkanDevice& device, VulkanSurface& surface, Window* window);
		~VulkanSwapchain();

		// 禁止拷贝和移动
		VulkanSwapchain(const VulkanSwapchain&) = delete;
		VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;
		VulkanSwapchain(VulkanSwapchain&&) = delete;
		VulkanSwapchain& operator=(VulkanSwapchain&&) = delete;

		VkResult AcquireNextImage(uint32_t* imageIndex, VkSemaphore semaphore);

		VkResult SubmitImage(uint32_t imageIndex, VkQueue presentQueue, VkSemaphore waitSemaphore);

		void Recreate(Window* window);

		VkSwapchainKHR GetHandle() const { return m_Swapchain; }
		VkFormat GetImageFormat() const { return m_ImageFormat; }
		VkExtent2D GetExtent() const { return m_Extent; }
		const std::vector<VkImageView>& GetImageViews() const { return m_ImageViews; }
		uint32_t GetImageCount() const { return static_cast<uint32_t>(m_Images.size()); }

	private:
		void Init(Window* window);
		void Cleanup();

		// 初始化过程中的详细步骤
		void ChooseSurfaceFormat();
		void ChoosePresentMode();
		void ChooseExtent(Window* window);
		void CreateSwapchain();
		void CreateImageViews();

	private:
		const VulkanDevice& m_Device; // 持有对设备的引用
		const VulkanSurface& m_Surface; // 持有对表面的引用
		VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;

		// 交换链资源
		std::vector<VkImage> m_Images;
		std::vector<VkImageView> m_ImageViews;
		VkFormat m_ImageFormat;
		VkExtent2D m_Extent;

		// 协商后选择的参数
		VkSurfaceFormatKHR m_SurfaceFormat;
		VkPresentModeKHR m_PresentMode;
	};
}