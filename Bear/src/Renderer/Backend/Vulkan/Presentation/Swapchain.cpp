#include "bearpch.h"

#include "Surface.h"
#include "Core/Device.h"
#include "Bear/Window.h"
#include "Swapchain.h"
#include "Pipeline/RenderPass.h"
#include "Pipeline/Framebuffer.h"
#include "Resources/Image.h"
#include "Core/Utils.h"
#include "Sync/Semaphore.h"
namespace Bear {

	Bear::Swapchain::Swapchain(Device& device, RenderPass& renderPass)
		:m_Device(device), m_RenderPass(renderPass)
	{
		Init();
#ifdef BEAR_DEBUG
		BEAR_CORE_INFO("Vulkan Swapchain created successfully.");
#endif // BEAR_DEBUG
	}

	Swapchain::~Swapchain()
	{
		Cleanup();
#ifdef BEAR_DEBUG
		BEAR_CORE_INFO("Vulkan Swapchain destroyed successfully.");
#endif // BEAR_DEBUG
	}

	void Swapchain::CreateFramebuffers(const RenderPass& renderPass)
	{
		m_Framebuffers.resize(m_ImageCount);

		for (size_t i = 0; i < m_ImageCount; ++i) {
			std::vector<VkImageView> attachments = { m_ImageViews[i], m_DepthImage->GetView()};
			m_Framebuffers[i] = std::make_unique<Framebuffer>(m_Device, renderPass, attachments, m_Extent.width, m_Extent.height);
		}
#ifdef BEAR_DEBUG
		BEAR_CORE_INFO("Vulkan Framebuffers created successfully.");
#endif // BEAR_DEBUG
	}

	/*VkResult Swapchain::AcquireNextImage(uint32_t* imageIndex, VkSemaphore semaphore)
	{
		BEAR_CORE_ASSERT(vkAcquireNextImageKHR(m_Device.GetDevice(), m_Swapchain, UINT64_MAX, semaphore, VK_NULL_HANDLE, imageIndex) == VK_SUCCESS, "Failed to acquire next image from swapchain.");
		return VK_SUCCESS;
	}*/
	uint32_t Swapchain::AcquireNextImage()
	{
		return 0;
	}
	void Swapchain::Present(uint32_t imageIndex)
	{
	}
	uint32_t Swapchain::AcquireNextImage(Semaphore& imageAvailableSemaphore)
	{
		uint32_t imageIndex = 0;
		VkResult result = vkAcquireNextImageKHR(m_Device.GetDevice(), m_Swapchain, UINT64_MAX, imageAvailableSemaphore.GetHandle(), VK_NULL_HANDLE, &imageIndex);
		if (result == VK_SUCCESS) {
			return imageIndex; // 成功获取图像索引
		}
		else if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR) {
			// 交换链可能需要重建
			BEAR_CORE_WARN("Swapchain is suboptimal or out of date, need to recreate swapchain.");
			Recreate();
		}
		return UINT32_MAX; // 获取图像索引失败或重建交换链，跳过该帧
	}

	void Swapchain::Present(uint32_t imageIndex, Semaphore& renderFinishedSemaphore)
	{
		VkQueue presentQueue = m_Device.GetPresentQueue();
		if (presentQueue == VK_NULL_HANDLE) {
			BEAR_CORE_ERROR("Failed to get present queue from Vulkan device.");
			return;
		}
		VkResult result = SubmitImage(imageIndex, presentQueue, renderFinishedSemaphore.GetHandle());
		if (result != VK_SUCCESS) {
			BEAR_CORE_ERROR("Failed to submit image to swapchain for presentation.");
			return;
		}
	}

	void Swapchain::Resize()
	{
		// 在窗口大小改变时调用，通常在窗口的resize事件中触发
		Recreate();
	}

	VkResult Swapchain::SubmitImage(uint32_t imageIndex, VkQueue presentQueue, VkSemaphore waitSemaphore)
	{
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &waitSemaphore; // 等待信号量
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_Swapchain; // 交换链
		presentInfo.pImageIndices = &imageIndex; // 图像索引

		BEAR_CORE_ASSERT(vkQueuePresentKHR(presentQueue, &presentInfo) == VK_SUCCESS, "Failed to present image to swapchain.");
		return VK_SUCCESS;
	}
	void Swapchain::Recreate()
	{
		// 在重建之前，等待设备空闲，确保所有资源都不在被使用
		vkDeviceWaitIdle(m_Device.GetDevice());
		Cleanup();
		
		// 重新初始化交换链
		Init();
	}

	void Swapchain::GetExtent(uint32_t& width, uint32_t& height) const
	{
		width = m_Extent.width;
		height = m_Extent.height;
		if (width == 0 || height == 0) {
			// 如果宽度或高度为0，可能是因为窗口未正确初始化或已最小化
			BEAR_CORE_WARN("Swapchain extent is zero, returning default values.");
			width = 800; // 默认宽度
			height = 600; // 默认高度
		}
	}

	void Swapchain::Init()
	{
		ChooseSurfaceFormat();
		ChoosePresentMode();
		ChooseExtent();
		CreateSwapchain();
		CreateImageViews();
		CreateDepthResources();
		CreateFramebuffers(m_RenderPass);
	}

	void Swapchain::Cleanup()
	{
		CleanupFramebuffers();
		if (m_DepthImage) {
			m_DepthImage.reset(); // 使用智能指针自动管理资源
		}
		for (auto imageView : m_ImageViews) {
			vkDestroyImageView(m_Device.GetDevice(), imageView, nullptr);
		}
		m_ImageViews.clear();
		if (m_Swapchain != VK_NULL_HANDLE) {
			vkDestroySwapchainKHR(m_Device.GetDevice(), m_Swapchain, nullptr);
			m_Swapchain = VK_NULL_HANDLE;
		}
	}

	void Swapchain::CleanupFramebuffers()
	{
		if (m_DepthImage) {
			m_DepthImage.reset(); // 使用智能指针自动管理资源
		}
		// 销毁所有帧缓冲区
		m_Framebuffers.clear();
#ifdef BEAR_DEBUG
		BEAR_CORE_INFO("Framebuffer destoryed successfully.");
#endif // BEAR_DEBUG

	}

	void Swapchain::ChooseSurfaceFormat()
	{
		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_Device.GetPhysicalDevice(), m_Device.GetSurface().GetHandle(), &formatCount, nullptr);
		BEAR_CORE_ASSERT(formatCount > 0, "No surface formats available.");
		std::vector<VkSurfaceFormatKHR> formats(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_Device.GetPhysicalDevice(), m_Device.GetSurface().GetHandle(), &formatCount, formats.data());

		// 选择最常用的表面格式（例如：VK_FORMAT_B8G8R8A8_SRGB）
		for (const auto& format : formats) {
			if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				m_SurfaceFormat = format;
				return;
			}
		}
		// 如果没有找到常用格式，选择第一个可用格式
		m_SurfaceFormat = formats[0];

	}
	void Swapchain::ChoosePresentMode()
	{
		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_Device.GetPhysicalDevice(), m_Device.GetSurface().GetHandle(), &presentModeCount, nullptr);
		BEAR_CORE_ASSERT(presentModeCount > 0, "No present modes available.");
		std::vector<VkPresentModeKHR> presentModes(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_Device.GetPhysicalDevice(), m_Device.GetSurface().GetHandle(), &presentModeCount, presentModes.data());
		// 选择最常用的呈现模式（例如：VK_PRESENT_MODE_FIFO_KHR）
		for (const auto& mode : presentModes) {
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
				m_PresentMode = mode;
				return;
			}
		}
		// 如果没有找到常用模式，选择第一个可用模式
		m_PresentMode = presentModes[0];
	}
	void Swapchain::ChooseExtent()
	{
		VkSurfaceCapabilitiesKHR capabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_Device.GetPhysicalDevice(), m_Device.GetSurface().GetHandle(), &capabilities);
		if (capabilities.currentExtent.width != UINT32_MAX) {
			// 如果表面具有固定的范围，则使用该范围
			m_Extent = capabilities.currentExtent;
		}
		else {
			// 否则，根据窗口大小计算范围
			int width, height;
			glfwGetFramebufferSize(static_cast<GLFWwindow*>(m_Device.GetSurface().GetNativeWindow()), &width, &height);

			m_Extent.width = std::clamp(
				static_cast<uint32_t>(width),
				capabilities.minImageExtent.width,
				capabilities.maxImageExtent.width
			);

			m_Extent.height = std::clamp(
				static_cast<uint32_t>(height),
				capabilities.minImageExtent.height,
				capabilities.maxImageExtent.height
			);

		}
	}
	void Swapchain::CreateSwapchain()
	{
		VkSurfaceCapabilitiesKHR capabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_Device.GetPhysicalDevice(), m_Device.GetSurface().GetHandle(), &capabilities);
		m_MinImageCount = capabilities.minImageCount + 1; // 至少需要一个图像
		if (capabilities.maxImageCount > 0 && m_MinImageCount > capabilities.maxImageCount) {
			m_MinImageCount = capabilities.maxImageCount;
		}
		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_Device.GetSurface().GetHandle();
		createInfo.minImageCount = m_MinImageCount;
		createInfo.imageFormat = m_SurfaceFormat.format;
		createInfo.imageColorSpace = m_SurfaceFormat.colorSpace;
		createInfo.imageExtent = m_Extent;
		createInfo.imageArrayLayers = 1; // 单层图像
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // 用于颜色附件
		createInfo.preTransform = capabilities.currentTransform; // 保持当前转换
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // 不透明合成
		createInfo.presentMode = m_PresentMode; // 使用选择的呈现模式
		createInfo.clipped = VK_TRUE; // 剪裁图像
		createInfo.oldSwapchain = VK_NULL_HANDLE; // 没有旧交换链
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.pNext = nullptr; // 没有额外的结构体

		QueueFamilyIndices indices = m_Device.GetQueueFamilyIndices();
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}

		BEAR_CORE_ASSERT(vkCreateSwapchainKHR(m_Device.GetDevice(), &createInfo, nullptr, &m_Swapchain) == VK_SUCCESS, "Failed to create Vulkan swapchain.");

		// 获取交换链图像
		uint32_t swapchainImageCount = 0;
		vkGetSwapchainImagesKHR(m_Device.GetDevice(), m_Swapchain, &swapchainImageCount, nullptr);
		BEAR_CORE_ASSERT(swapchainImageCount > 0, "No swapchain images available.");
		m_ImageCount = swapchainImageCount;
		m_Images.resize(m_ImageCount);
		vkGetSwapchainImagesKHR(m_Device.GetDevice(), m_Swapchain, &swapchainImageCount, m_Images.data());

		m_ImageFormat = m_SurfaceFormat.format;
	}
	void Swapchain::CreateImageViews()
	{
		m_ImageViews.resize(m_Images.size());
		for (size_t i = 0; i < m_Images.size(); ++i) {
			VkImageViewCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = m_Images[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = m_ImageFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			BEAR_CORE_ASSERT(vkCreateImageView(m_Device.GetDevice(), &createInfo, nullptr, &m_ImageViews[i]) == VK_SUCCESS, "Failed to create image view.");
		}
	}
	void Swapchain::CreateDepthResources()
	{
		VkFormat depthFormat = FindDepthFormat(m_Device);

		m_DepthImage = std::make_unique<Image>(m_Device, m_Extent.width, m_Extent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, 
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	}
	
}