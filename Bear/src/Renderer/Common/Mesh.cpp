#include "bearpch.h"

#include "Mesh.h"
#include "ModelLoader.h"
#include "Command/CommandBuffer.h"
#include "Resources/Buffer.h" 
#include "Core/Device.h"


namespace Bear {
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
	Mesh::~Mesh()
	{
		m_VertexBuffer.reset();
		m_IndexBuffer.reset();
	}
	void Mesh::Bind(RHICommandList& cmd) const
	{
		cmd.BindVertexBuffer(*m_VertexBuffer, 0, 0);
		cmd.BindIndexBuffer(*m_IndexBuffer, 0);
	}
	void Mesh::Draw(RHICommandList& cmd) const
	{
		// cmd.Draw(m_IndexCount, 1, 0, 0);
		cmd.DrawIndexed(m_IndexCount, 1, 0, 0, 0);
	}
}