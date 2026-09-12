#pragma once
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h" // vma
#include "RHI/RHIResources.h"
#include "RHI/RHITypes.h"

namespace Bear {

	class Device;

	class Image : public RHIImage {

	public:
		Image(const Device& device, uint32_t width, uint32_t height, VkFormat format,
			VkImageTiling tiling,VkImageUsageFlags usage, VmaMemoryUsage memoryUsage, uint32_t mipLevels = 1);
		// wraps an externally owned image/view (e.g. swapchain images); does not destroy them
		Image(const Device& device, VkImage image, VkImageView view, uint32_t width, uint32_t height, VkFormat format);
		~Image() override;


		Image(const Image&) = delete;
		Image& operator=(const Image&) = delete;
		Image(Image&&) = delete;
		Image& operator=(Image&&) = delete;

		uint32_t GetWidth() const override;
		uint32_t GetHeight() const override;
		uint32_t GetMipLevels() const override;
		PixelFormat GetFormat() const override;
		void* GetMipView(uint32_t mipLevel) const override { return reinterpret_cast<void*>(GetMipViewVk(mipLevel)); }

		inline VkImageView GetView() const { return m_ImageView; }
		inline VkImage GetImage() const { return m_Image; }
		// per-mip image view (levelCount = 1); falls back to the full view when out of range
		VkImageView GetMipViewVk(uint32_t mipLevel) const;
		VkImageAspectFlags GetAspectMask() const { return GetAspectMask(m_Format); }
		// inline VkFormat GetFormat() const { return m_Format; }

	private:
		const Device& m_Device;
		VkImage m_Image = VK_NULL_HANDLE;
		//VkDeviceMemory m_Memory = VK_NULL_HANDLE;
		VkImageView m_ImageView = VK_NULL_HANDLE;
		std::vector<VkImageView> m_MipViews;
		VkFormat m_Format = VK_FORMAT_UNDEFINED;

		VmaAllocation m_Allocation = VK_NULL_HANDLE; // vma
		bool m_OwnsResources = true;
		uint32_t m_Width = 0;
		uint32_t m_Height = 0;
		uint32_t m_MipLevels = 1;
	private:
		void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage);
		void CreateImageView(VkFormat format);
		void CreateMipViews();
		void Cleanup();
		static VkImageAspectFlags GetAspectMask(VkFormat format);
	};
}
