#pragma once
#include <cstdint>
namespace Bear {

	class RHIFramebuffer;
	class RHISwapchain
	{
	public:
		virtual ~RHISwapchain() = default;

		virtual uint32_t AcquireNextImage() = 0;
		virtual void Present(uint32_t imageIndex) = 0;

		//virtual void Resize(uint32_t newWidth, uint32_t newHeight) = 0;
		virtual void Resize() = 0;
		//virtual RHIFramebuffer* GetFramebuffer() const = 0;
		virtual RHIFramebuffer* GetFramebuffer(uint32_t index) const = 0;
		virtual 
		// virtual RHIImage* GetImage(uint32_t index) const = 0; // δ���������
		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetImageCount() const = 0;
		virtual void GetExtent(uint32_t& width, uint32_t& height) const = 0;
	};
}