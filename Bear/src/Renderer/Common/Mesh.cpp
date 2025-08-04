#include "bearpch.h"

#include "Mesh.h"
#include "Command/CommandBuffer.h"
#include "Resources/Buffer.h" 
#include "Core/Types.h"
#include "Core/Device.h"
#include "RHI/RHITypes.h"

namespace Bear {
	Mesh::Mesh(RHIDevice& device, const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices)
	{
		//m_VertexBuffer = std::make_unique<Buffer>(device, sizeof(Vertex) * vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		m_VertexBuffer = device.CreateBuffer(sizeof(Vertex) * vertices.size(), BufferUsage::VertexBuffer, true);
		//m_IndexBuffer = std::make_unique<Buffer>(device, sizeof(uint16_t) * indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		m_IndexBuffer = device.CreateBuffer(sizeof(uint16_t) * indices.size(), BufferUsage::IndexBuffer, true);
		m_IndexCount = static_cast<uint32_t>(indices.size());

		m_VertexBuffer->UploadData(vertices.data(), sizeof(Vertex) * vertices.size());
		m_IndexBuffer->UploadData(indices.data(), sizeof(uint16_t) * indices.size());
	}
	Mesh::~Mesh()
	{
		m_VertexBuffer.reset(); // 清理顶点缓冲区
		m_IndexBuffer.reset(); // 清理索引缓冲区
	}
	void Mesh::Bind(RHICommandList& commandBuffer) const
	{
		Buffer* vkVertexBuffer = static_cast<Buffer*>(m_VertexBuffer.get());
		Buffer* vkIndexBuffer = static_cast<Buffer*>(m_IndexBuffer.get());
		//VkBuffer vertexBuffers[] = { vkVertexBuffer->GetHandle() };
		// std::vector<Buffer*> vertexBuffers = { static_cast<Buffer*>(m_VertexBuffer.get())};
		std::vector<VkDeviceSize> offsets = { 0 };
		// 这两个函数设计的不太好
		commandBuffer.BindVertexBuffer(*vkVertexBuffer, 0, 0);
		commandBuffer.BindIndexBuffer(*vkIndexBuffer, 0);
	}
	void Mesh::Draw(RHICommandList& commandBuffer) const
	{
		commandBuffer.DrawIndexed(m_IndexCount, 1, 0, 0, 0);
	}
}