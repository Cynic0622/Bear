#include "bearpch.h"

#include "Mesh.h"
#include "ModelLoader.h"
#include "Command/CommandBuffer.h"

namespace Bear {

	RHIBuffer* Mesh::s_GlobalVertexBuffer = nullptr;
	RHIBuffer* Mesh::s_GlobalIndexBuffer = nullptr;

	Mesh::Mesh(RHIDevice& device, const std::string& path)
	{
		ModelData modelData = LoadModelDataFromFile(path);
		m_IndexCount = static_cast<uint32_t>(modelData.indices.size());

		size_t vertexBufferSize = modelData.vertices.size() * sizeof(Vertex);
		size_t indexBufferSize = modelData.indices.size() * sizeof(uint32_t);

		m_VertexBuffer = device.CreateBuffer(vertexBufferSize, BufferUsage::VertexBuffer, true);
		m_VertexBuffer->UploadData(modelData.vertices.data(), vertexBufferSize);
		m_IndexBuffer = device.CreateBuffer(indexBufferSize, BufferUsage::IndexBuffer, true);
		m_IndexBuffer->UploadData(modelData.indices.data(), indexBufferSize);
	}
	Mesh::Mesh(RHIDevice& device, const std::vector<VertexDescription>& vertices, const std::vector<uint32_t>& indices)
	{
		size_t vertexBufferSize = vertices.size() * sizeof(VertexDescription);
		size_t indexBufferSize = indices.size() * sizeof(uint32_t);
		m_IndexCount = static_cast<uint32_t>(indices.size());

		m_VertexBuffer = device.CreateBuffer(vertexBufferSize, BufferUsage::VertexBuffer, true);
		m_VertexBuffer->UploadData(vertices.data(), vertexBufferSize);
		m_IndexBuffer = device.CreateBuffer(indexBufferSize, BufferUsage::IndexBuffer, true);
		m_IndexBuffer->UploadData(indices.data(), indexBufferSize);

		glm::vec3 minValue = vertices.front().position;
		glm::vec3 maxValue = minValue;
		for (const auto& vert : vertices) {
			minValue = glm::min(minValue, vert.position);
			maxValue = glm::max(maxValue, vert.position);
		}
		m_AABB = AABB(minValue, maxValue);
	}
	Mesh::~Mesh()
	{
		m_VertexBuffer.reset();
		m_IndexBuffer.reset();
	}
	
	void Mesh::Draw(RHICommandList& cmd) const
	{
		if (s_GlobalVertexBuffer && s_GlobalIndexBuffer && m_GlobalVertexOffset >= 0)
		{
			cmd.BindVertexBuffer(*s_GlobalVertexBuffer, 0, m_GlobalVertexOffset * sizeof(VertexDescription));
			cmd.BindIndexBuffer(*s_GlobalIndexBuffer, m_GlobalFirstIndex * sizeof(uint32_t));
			cmd.DrawIndexed(m_IndexCount, 1, 0, 0, 0);
		}
		else
		{
			cmd.BindVertexBuffer(*m_VertexBuffer, 0, 0);
			cmd.BindIndexBuffer(*m_IndexBuffer, 0);
			cmd.DrawIndexed(m_IndexCount, 1, 0, 0, 0);
		}
	}

	void Mesh::SetupIndirect(int32_t globalVertexOffset, int32_t globalFirstIndex)
	{
		m_GlobalVertexOffset = globalVertexOffset;
		m_GlobalFirstIndex = globalFirstIndex;
	}

	void Mesh::SetGlobalBuffers(RHIBuffer* vertexBuffer, RHIBuffer* indexBuffer)
	{
		s_GlobalVertexBuffer = vertexBuffer;
		s_GlobalIndexBuffer = indexBuffer;
	}
}