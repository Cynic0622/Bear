#include "bearpch.h"

#include "VulkanBuffer.h"
#include "Core/VulkanDevice.h"

namespace Bear {

	VulkanBuffer::VulkanBuffer(const VulkanDevice& device, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
		:m_Device(device), m_Size(size)
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // 单队列，所以使用独占模式
		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = memoryUsage;
		
		//auto t = vmaCreateBuffer(m_Device.GetAllocator(), &bufferInfo, &allocInfo, &m_Buffer, &m_Allocation, nullptr);
		BEAR_CORE_ASSERT(vmaCreateBuffer(m_Device.GetAllocator(), &bufferInfo, &allocInfo, &m_Buffer, &m_Allocation, nullptr) == VK_SUCCESS,
			"Failed to create buffer using VMA!");
	}
	VulkanBuffer::~VulkanBuffer()
	{
		if (m_Buffer != VK_NULL_HANDLE)
		{
			vmaDestroyBuffer(m_Device.GetAllocator(), m_Buffer, m_Allocation);
			m_Buffer = VK_NULL_HANDLE;
			m_Allocation = VK_NULL_HANDLE;
		}
	}
	void* VulkanBuffer::Map()
	{
		if (m_MappedData)
		{
			return m_MappedData;
		}
		BEAR_CORE_ASSERT(vmaMapMemory(m_Device.GetAllocator(), m_Allocation, &m_MappedData) == VK_SUCCESS,
			"Failed to map buffer memory!");
		return m_MappedData;
	}
	void VulkanBuffer::Unmap()
	{
		if (m_MappedData)
		{
			vmaUnmapMemory(m_Device.GetAllocator(), m_Allocation);
			m_MappedData = nullptr;
		}
	}
	void VulkanBuffer::UploadData(const void* data, size_t size, size_t offset)
	{
		BEAR_CORE_ASSERT(size <= m_Size, "Data size exceeds buffer size!");
		void* mappedData = Map();
		memcpy(mappedData, data, size);
		// 如果内存不是 HOST_COHERENT，需要手动 flush
		vmaFlushAllocation(m_Device.GetAllocator(), m_Allocation, 0, size);
		Unmap();
	}
}