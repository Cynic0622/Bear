#pragma once
#include "bearpch.h"

namespace Bear {
	// 纯虚函数类，用于表示RHI资源
	class RHIBuffer {
		public:
		virtual ~RHIBuffer() = default;
		virtual void* Map() = 0;
		virtual void Unmap() = 0;
		virtual void UploadData(const void* data, size_t size, size_t offset = 0) = 0;
	};
}
