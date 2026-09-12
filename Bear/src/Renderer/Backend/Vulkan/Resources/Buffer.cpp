#include "bearpch.h"

#include "Buffer.h"
#include "Core/Device.h"

namespace Bear {

	Buffer::Buffer(const Device& device, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
		:m_Device(device), m_Size(size)
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // �����У�����ʹ�ö�ռģʽ
		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = memoryUsage;
		
		//auto t = vmaCreateBuffer(m_Device.GetAllocator(), &bufferInfo, &allocInfo, &m_Buffer, &m_Allocation, nullptr);
		BEAR_CORE_ASSERT(vmaCreateBuffer(m_Device.GetAllocator(), &bufferInfo, &allocInfo, &m_Buffer, &m_Allocation, nullptr) == VK_SUCCESS,
			"Failed to create buffer using VMA!");
	}
	Buffer::~Buffer()
	{
		if (m_Buffer != VK_NULL_HANDLE)
		{
			vmaDestroyBuffer(m_Device.GetAllocator(), m_Buffer, m_Allocation);
			m_Buffer = VK_NULL_HANDLE;
			m_Allocation = VK_NULL_HANDLE;
		}
	}
	void* Buffer::Map()
	{
		if (m_MappedData)
		{
			return m_MappedData;
		}
		BEAR_CORE_ASSERT(vmaMapMemory(m_Device.GetAllocator(), m_Allocation, &m_MappedData) == VK_SUCCESS,
			"Failed to map buffer memory!");
		return m_MappedData;
	}
	void Buffer::Unmap()
	{
		if (m_MappedData)
		{
			vmaUnmapMemory(m_Device.GetAllocator(), m_Allocation);
			m_MappedData = nullptr;
		}
	}
	void Buffer::UploadData(const void* data, size_t size, size_t offset)
	{
		BEAR_CORE_ASSERT(size <= m_Size, "Data size exceeds buffer size!");
		void* mappedData = Map();
		memcpy(static_cast<char*>(mappedData) + offset, data, size);
		// ����ڴ治�� HOST_COHERENT����Ҫ�ֶ� flush
		vmaFlushAllocation(m_Device.GetAllocator(), m_Allocation, offset, size);
		Unmap();
	}
	void Buffer::ReadData(void* dst, size_t size, size_t offset)
	{
		BEAR_CORE_ASSERT(size + offset <= m_Size, "Read size exceeds buffer size!");
		void* mappedData = Map();
		// make GPU writes visible to the CPU on non-coherent memory
		vmaInvalidateAllocation(m_Device.GetAllocator(), m_Allocation, offset, size);
		memcpy(dst, static_cast<char*>(mappedData) + offset, size);
		Unmap();
	}
}