#pragma once
#include "bearpch.h"
#include "RHITypes.h"
namespace Bear {

	class RHIBuffer {
		public:
		virtual ~RHIBuffer() = default;
		virtual void* Map() = 0;
		virtual void Unmap() = 0;
		virtual void UploadData(const void* data, size_t size, size_t offset = 0) = 0;
		// reads back GPU-written data (invalidates caches so non-coherent memory is visible)
		virtual void ReadData(void* dst, size_t size, size_t offset = 0) = 0;
		virtual size_t GetSize() const = 0;
	};

	class RHIImage
	{
		public:
		virtual ~RHIImage() = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetMipLevels() const = 0;
		virtual PixelFormat GetFormat() const = 0;
		// returns the backend image-view handle for the given mip level (levelCount = 1)
		virtual void* GetMipView(uint32_t mipLevel) const = 0;
	};

	class RHISampler {
	public:
		virtual ~RHISampler() = default;
	};
}
