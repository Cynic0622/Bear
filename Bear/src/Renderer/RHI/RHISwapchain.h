#pragma once
#include <cstdint>
namespace Bear {

	class RHISwapchain
	{
	public:
		virtual ~RHISwapchain() = default;

		virtual uint32_t AcquireNextImage() = 0;
		virtual void Present(uint32_t imageIndex) = 0;

		//virtual void Resize(uint32_t newWidth, uint32_t newHeight) = 0;
		virtual void Resize() = 0;
		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetImageCount() const = 0;
		virtual void GetExtent(uint32_t& width, uint32_t& height) const = 0;
		virtual void* GetDepthView(uint32_t imageIndex) const = 0;
		virtual RHIImage* GetDepthImage(uint32_t imageIndex) const = 0;
		virtual RHIImage* GetColorImage(uint32_t imageIndex) const = 0;
		virtual void* GetColorView(uint8_t index) const = 0;
	};
}