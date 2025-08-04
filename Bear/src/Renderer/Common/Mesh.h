#pragma once

#include <vector>
#include "RHI/RHIResources.h"
#include "RHI/RHIDevice.h"

namespace Bear {

	class Device;
	// Vertex structure for the mesh
	struct Vertex;
	class Mesh {
	
	public:
		Mesh(RHIDevice& device, const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices);
		~Mesh();

		void Bind(RHICommandList& commandBuffer) const;
		void Draw(RHICommandList& commandBuffer) const;

	private:
		//std::unique_ptr<Buffer> m_VertexBuffer;
		std::unique_ptr<RHIBuffer> m_VertexBuffer;
		//std::unique_ptr<Buffer> m_IndexBuffer;
		std::unique_ptr<RHIBuffer> m_IndexBuffer;
		uint32_t m_IndexCount;

	};
}