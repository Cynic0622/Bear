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

		for (size_t i = 0; i < m_ImageCount; ++i)
		{
			std::vector<VkImageView> attachments = { m_ImageViews[i], m_DepthImages[i]->GetView()};
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
			return imageIndex; // �ɹ���ȡͼ������
		}
		else if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR) {
			// ������������Ҫ�ؽ�
			BEAR_CORE_WARN("Swapchain is suboptimal or out of date, need to recreate swapchain.");
			Recreate();
		}
		return UINT32_MAX; // ��ȡͼ������ʧ�ܻ��ؽ���������������֡
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
		// �ڴ��ڴ�С�ı�ʱ���ã�ͨ���ڴ��ڵ�resize�¼��д���
		Recreate();
	}

	VkResult Swapchain::SubmitImage(uint32_t imageIndex, VkQueue presentQueue, VkSemaphore waitSemaphore)
	{
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &waitSemaphore; // �ȴ��ź���
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_Swapchain; // ������
		presentInfo.pImageIndices = &imageIndex; // ͼ������

		BEAR_CORE_ASSERT(vkQueuePresentKHR(presentQueue, &presentInfo) == VK_SUCCESS, "Failed to present image to swapchain.");
		return VK_SUCCESS;
	}
	void Swapchain::Recreate()
	{
		// ���ؽ�֮ǰ���ȴ��豸���У�ȷ��������Դ�����ڱ�ʹ��
		vkDeviceWaitIdle(m_Device.GetDevice());
		Cleanup();
		
		// ���³�ʼ��������
		Init();
	}

	void Swapchain::GetExtent(uint32_t& width, uint32_t& height) const
	{
		width = m_Extent.width;
		height = m_Extent.height;
		if (width == 0 || height == 0) {
			// ������Ȼ�߶�Ϊ0����������Ϊ����δ��ȷ��ʼ��������С��
			BEAR_CORE_WARN("Swapchain extent is zero, returning default values.");
			width = 800; // Ĭ�Ͽ���
			height = 600; // Ĭ�ϸ߶�
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
		for (auto& depthImage : m_DepthImages) {
			depthImage.reset();
		}
		m_DepthImages.clear();
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
		for (auto& depthImage : m_DepthImages) {
			depthImage.reset();
		}
		m_DepthImages.clear();
		// ��������֡������
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

		// ѡ����õı����ʽ�����磺VK_FORMAT_B8G8R8A8_SRGB��
		for (const auto& format : formats) {
			if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				m_SurfaceFormat = format;
				return;
			}
		}
		// ���û���ҵ����ø�ʽ��ѡ���һ�����ø�ʽ
		m_SurfaceFormat = formats[0];

	}
	void Swapchain::ChoosePresentMode()
	{
		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_Device.GetPhysicalDevice(), m_Device.GetSurface().GetHandle(), &presentModeCount, nullptr);
		BEAR_CORE_ASSERT(presentModeCount > 0, "No present modes available.");
		std::vector<VkPresentModeKHR> presentModes(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_Device.GetPhysicalDevice(), m_Device.GetSurface().GetHandle(), &presentModeCount, presentModes.data());
		// ѡ����õĳ���ģʽ�����磺VK_PRESENT_MODE_FIFO_KHR��
		for (const auto& mode : presentModes) {
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
				m_PresentMode = mode;
				return;
			}
		}
		// ���û���ҵ�����ģʽ��ѡ���һ������ģʽ
		m_PresentMode = presentModes[0];
	}
	void Swapchain::ChooseExtent()
	{
		VkSurfaceCapabilitiesKHR capabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_Device.GetPhysicalDevice(), m_Device.GetSurface().GetHandle(), &capabilities);
		if (capabilities.currentExtent.width != UINT32_MAX) {
			// ���������й̶��ķ�Χ����ʹ�ø÷�Χ
			m_Extent = capabilities.currentExtent;
		}
		else {
			// ���򣬸��ݴ��ڴ�С���㷶Χ
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
		m_MinImageCount = capabilities.minImageCount + 1; // ������Ҫһ��ͼ��
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
		createInfo.imageArrayLayers = 1; // ����ͼ��
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // ������ɫ����
		createInfo.preTransform = capabilities.currentTransform; // ���ֵ�ǰת��
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // ��͸���ϳ�
		createInfo.presentMode = m_PresentMode; // ʹ��ѡ��ĳ���ģʽ
		createInfo.clipped = VK_TRUE; // ����ͼ��
		createInfo.oldSwapchain = VK_NULL_HANDLE; // û�оɽ�����
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.pNext = nullptr; // û�ж���Ľṹ��

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

		// ��ȡ������ͼ��
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

		m_DepthImages.clear();
		for (uint32_t i = 0; i < m_ImageCount; ++i)
		{
			m_DepthImages.push_back(std::make_unique<Image>(m_Device, m_Extent.width, m_Extent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_GPU_ONLY));
		}
	}
	
}