#pragma once
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h" // vma
namespace Bear {

	class Device;

	class Image {

	public:
		Image(const Device& device, uint32_t width, uint32_t height, VkFormat format, 
			VkImageTiling tiling,VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VmaMemoryUsage memoryUsage);
		~Image();

		// 禁止拷贝和移动
		Image(const Image&) = delete;
		Image& operator=(const Image&) = delete;
		Image(Image&&) = delete;
		Image& operator=(Image&&) = delete;

		inline VkImageView GetView() const { return m_ImageView; }
		inline VkImage GetImage() const { return m_Image; }
		inline VkFormat GetFormat() const { return m_Format; }

	private:
		const Device& m_Device;
		VkImage m_Image = VK_NULL_HANDLE;
		//VkDeviceMemory m_Memory = VK_NULL_HANDLE;
		VkImageView m_ImageView = VK_NULL_HANDLE;
		VkFormat m_Format = VK_FORMAT_UNDEFINED;
		// 修改：使用vma来管理内存
		VmaAllocation m_Allocation = VK_NULL_HANDLE; // VMA分配器的分配句柄
	private:
		void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VmaMemoryUsage memoryUsage);
		void CreateImageView(VkFormat format);
		void Cleanup();
		VkImageAspectFlags GetAspectMask(VkFormat format);
	};
}