#include "bearpch.h"

#include "VulkanImage.h"
#include "Core/VulkanDevice.h"

namespace Bear {
	VulkanImage::VulkanImage(const VulkanDevice& device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, 
		VkImageUsageFlags usage, VkMemoryPropertyFlags properties)
		: m_Device(device), m_Format(format)
	{
		CreateImage(width, height, format, tiling, usage, properties);

		CreateImageView(usage);

	}

	VulkanImage::~VulkanImage()
	{
		if (m_ImageView != VK_NULL_HANDLE) {
			vkDestroyImageView(m_Device.GetHandle(), m_ImageView, nullptr);
		}
		if (m_Image != VK_NULL_HANDLE) {
			vkDestroyImage(m_Device.GetHandle(), m_Image, nullptr);
		}
		if (m_Memory != VK_NULL_HANDLE) {
			vkFreeMemory(m_Device.GetHandle(), m_Memory, nullptr);
		}
	}

	void VulkanImage::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties)
	{
		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.flags = 0;

		BEAR_CORE_ASSERT(vkCreateImage(m_Device.GetHandle(), &imageInfo, nullptr, &m_Image) == VK_SUCCESS, "Failed to create Vulkan image!");

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(m_Device.GetHandle(), m_Image, &memRequirements);
		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = m_Device.FindMemoryType(memRequirements.memoryTypeBits, properties);

		BEAR_CORE_ASSERT(allocInfo.memoryTypeIndex != UINT32_MAX, "Failed to find suitable memory type for Vulkan image!");
		BEAR_CORE_ASSERT(vkAllocateMemory(m_Device.GetHandle(), &allocInfo, nullptr, &m_Memory) == VK_SUCCESS, "Failed to allocate Vulkan image memory!");

		vkBindImageMemory(m_Device.GetHandle(), m_Image, m_Memory, 0);
	}

	void VulkanImage::CreateImageView(VkImageUsageFlags usage)
	{
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_Image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = m_Format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // Adjust based on usage
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;
		if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		BEAR_CORE_ASSERT(vkCreateImageView(m_Device.GetHandle(), &viewInfo, nullptr, &m_ImageView) == VK_SUCCESS, "Failed to create Vulkan image view!");
	}
}