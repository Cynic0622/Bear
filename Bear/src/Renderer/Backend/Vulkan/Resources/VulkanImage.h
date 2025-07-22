#pragma once
#include <vulkan/vulkan.h>

namespace Bear {

	class VulkanDevice;

	class VulkanImage {

	public:
		VulkanImage(
			const VulkanDevice& device,
			uint32_t width,
			uint32_t height,
			VkFormat format,
			VkImageTiling tiling,
			VkImageUsageFlags usage,
			VkMemoryPropertyFlags properties
		);
		~VulkanImage();

		// ½ûÖ¹¿½±´ºÍÒÆ¶¯
		VulkanImage(const VulkanImage&) = delete;
		VulkanImage& operator=(const VulkanImage&) = delete;
		VulkanImage(VulkanImage&&) = delete;
		VulkanImage& operator=(VulkanImage&&) = delete;

		inline VkImageView GetView() const { return m_ImageView; }
		inline VkImage GetImage() const { return m_Image; }
		inline VkFormat GetFormat() const { return m_Format; }

	private:
		const VulkanDevice& m_Device;
		VkImage m_Image = VK_NULL_HANDLE;
		VkDeviceMemory m_Memory = VK_NULL_HANDLE;
		VkImageView m_ImageView = VK_NULL_HANDLE;
		VkFormat m_Format = VK_FORMAT_UNDEFINED;
	private:
		void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
		void CreateImageView(VkImageUsageFlags usage);
		void Cleanup();
	};
}