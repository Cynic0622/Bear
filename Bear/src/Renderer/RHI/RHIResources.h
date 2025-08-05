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
	};

	class RHITexture
	{
		public:
		virtual ~RHITexture() = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual PixelFormat GetFormat() const = 0;
	};

	class RHISampler {
	public:
		virtual ~RHISampler() = default;
	};
}
