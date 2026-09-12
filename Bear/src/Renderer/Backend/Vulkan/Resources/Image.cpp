#include "bearpch.h"

#include "Image.h"

#include "Utils.h"
#include "Core/Device.h"

namespace Bear {
	Image::Image(const Device& device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, 
		VkImageUsageFlags usage, VmaMemoryUsage memoryUsage, uint32_t mipLevels)
		: m_Device(device), m_Format(format), m_Width(width), m_Height(height), m_MipLevels(mipLevels)
	{
		CreateImage(width, height, format, tiling, usage, memoryUsage);

		CreateImageView(m_Format);
		CreateMipViews();

	}

	Image::~Image()
	{
		for (VkImageView view : m_MipViews) {
			vkDestroyImageView(m_Device.GetDevice(), view, nullptr);
		}
		m_MipViews.clear();
		if (m_ImageView != VK_NULL_HANDLE) {
			vkDestroyImageView(m_Device.GetDevice(), m_ImageView, nullptr);
		}
		if (m_Image != VK_NULL_HANDLE) {

			vmaDestroyImage(m_Device.GetAllocator(), m_Image, m_Allocation); // vma
		}

		/*if (m_Memory != VK_NULL_HANDLE) {
			vkFreeMemory(m_Device.GetDevice(), m_Memory, nullptr);
		}*/
	}

	uint32_t Image::GetWidth() const
	{
		return m_Width;
	}

	uint32_t Image::GetHeight() const
	{
		return m_Height;
	}

	uint32_t Image::GetMipLevels() const
	{
		return m_MipLevels;
	}

	PixelFormat Image::GetFormat() const
	{
		return FromVulkanFormat(m_Format);
	}

	void Image::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage)
	{
		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = m_MipLevels;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.flags = 0;

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = memoryUsage;

		BEAR_CORE_ASSERT(vmaCreateImage(m_Device.GetAllocator(), &imageInfo, &allocInfo, &m_Image, &m_Allocation, nullptr) == VK_SUCCESS,
			"Failed to create Vulkan image with VMA!");
	}

	void Image::CreateImageView(VkFormat format)
	{
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_Image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = m_Format;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = m_MipLevels;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;
		viewInfo.subresourceRange.aspectMask = GetAspectMask(format); // Determine aspect mask based on format
		BEAR_CORE_ASSERT(vkCreateImageView(m_Device.GetDevice(), &viewInfo, nullptr, &m_ImageView) == VK_SUCCESS, "Failed to create Vulkan image view!");
	}

	void Image::CreateMipViews()
	{
		m_MipViews.resize(m_MipLevels, VK_NULL_HANDLE);
		for (uint32_t level = 0; level < m_MipLevels; ++level)
		{
			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = m_Image;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = m_Format;
			viewInfo.subresourceRange.baseMipLevel = level;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;
			viewInfo.subresourceRange.aspectMask = GetAspectMask(m_Format);
			BEAR_CORE_ASSERT(vkCreateImageView(m_Device.GetDevice(), &viewInfo, nullptr, &m_MipViews[level]) == VK_SUCCESS, "Failed to create per-mip image view!");
		}
	}

	VkImageView Image::GetMipViewVk(uint32_t mipLevel) const
	{
		if (mipLevel < m_MipViews.size())
			return m_MipViews[mipLevel];
		return m_ImageView;
	}

	VkImageAspectFlags Image::GetAspectMask(VkFormat format)
	{
		switch (format) {
			// ����ȸ�ʽ
		case VK_FORMAT_D16_UNORM:
		case VK_FORMAT_D32_SFLOAT:
			return VK_IMAGE_ASPECT_DEPTH_BIT;

			// ���+ģ���ʽ
		case VK_FORMAT_D16_UNORM_S8_UINT:
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

			// ��ģ���ʽ
		case VK_FORMAT_S8_UINT:
			return VK_IMAGE_ASPECT_STENCIL_BIT;

			// Ĭ����ɫ��ʽ
		default:
			return VK_IMAGE_ASPECT_COLOR_BIT;
		}
		BEAR_CORE_ASSERT(false, "Unsupported format for aspect mask retrieval!");
	}
}