#pragma once
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h" // vma
#include "RHI/RHIResources.h"
#include "RHI/RHITypes.h"

namespace Bear {

	class Device;

	class Image : public RHITexture {

	public:
		Image(const Device& device, uint32_t width, uint32_t height, VkFormat format,
			VkImageTiling tiling,VkImageUsageFlags usage, VmaMemoryUsage memoryUsage);
		~Image() override;


		Image(const Image&) = delete;
		Image& operator=(const Image&) = delete;
		Image(Image&&) = delete;
		Image& operator=(Image&&) = delete;

		uint32_t GetWidth() const override;
		uint32_t GetHeight() const override;
		PixelFormat GetFormat() const override;

		inline VkImageView GetView() const { return m_ImageView; }
		inline VkImage GetImage() const { return m_Image; }
		// inline VkFormat GetFormat() const { return m_Format; }

	private:
		const Device& m_Device;
		VkImage m_Image = VK_NULL_HANDLE;
		//VkDeviceMemory m_Memory = VK_NULL_HANDLE;
		VkImageView m_ImageView = VK_NULL_HANDLE;
		VkFormat m_Format = VK_FORMAT_UNDEFINED;

		VmaAllocation m_Allocation = VK_NULL_HANDLE; // vma
		uint32_t m_Width = 0;
		uint32_t m_Height = 0;
	private:
		void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage);
		void CreateImageView(VkFormat format);
		void Cleanup();
		VkImageAspectFlags GetAspectMask(VkFormat format);
	};
}
