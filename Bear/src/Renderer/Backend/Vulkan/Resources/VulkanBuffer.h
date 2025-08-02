#pragma once

#include "vk_mem_alloc.h"
#include "RHI/RHIResources.h"

namespace Bear {

	class VulkanDevice;

	class VulkanBuffer : public RHIBuffer
	{
	public:
		VulkanBuffer(const VulkanDevice& device, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
		~VulkanBuffer() override;

		VulkanBuffer(const VulkanBuffer&) = delete;
		VulkanBuffer& operator=(const VulkanBuffer&) = delete;

		// --- 实现 RHI 接口 ---
		void* Map() override;
		void Unmap() override;
		// 将数据从CPU拷贝到Buffer
		void UploadData(const void* data, size_t size, size_t offset = 0) override;

		VkBuffer GetHandle() const { return m_Buffer; }
		VkDeviceSize GetSize() const { return m_Size; }

	private:
		const VulkanDevice& m_Device;
		VkBuffer m_Buffer;
		VmaAllocation m_Allocation = VK_NULL_HANDLE;
		VkDeviceSize m_Size;

		void* m_MappedData = nullptr;
	};
}