#pragma once

#include "vk_mem_alloc.h"
#include "RHI/RHIResources.h"

namespace Bear {

	class Device;

	class Buffer : public RHIBuffer
	{
	public:
		Buffer(const Device& device, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
		~Buffer() override;

		Buffer(const Buffer&) = delete;
		Buffer& operator=(const Buffer&) = delete;

		// --- rhi ---
		void* Map() override;
		void Unmap() override;

		void UploadData(const void* data, size_t size, size_t offset = 0) override;

		VkBuffer GetHandle() const { return m_Buffer; }
		size_t GetSize() const override { return static_cast<size_t>(m_Size); }

	private:
		const Device& m_Device;
		VkBuffer m_Buffer;
		VmaAllocation m_Allocation = VK_NULL_HANDLE;
		VkDeviceSize m_Size;

		void* m_MappedData = nullptr;
	};
}