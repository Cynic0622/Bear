#pragma once

#include "ResourceManager.h"
#include "RHI/RHIResources.h"
#include "RHI/RHIDevice.h"
#include "Scene/Frustum.h"

namespace Bear {
	struct AABB;

	class Device;
	// Vertex structure for the mesh
	struct VertexDescription;;
	class Mesh {
	
	public:
		Mesh(RHIDevice& device, const std::string& path);
		Mesh(RHIDevice& device, const std::vector<VertexDescription>& vertices, const std::vector<uint32_t>& indices);
		~Mesh();

		void Bind(RHICommandList& cmd) const;
		void Draw(RHICommandList& cmd) const;

		// Getters
		const AABB& GetAABB() const { return m_AABB; }

	private:
		//std::unique_ptr<Buffer> m_VertexBuffer;
		std::unique_ptr<RHIBuffer> m_VertexBuffer;
		//std::unique_ptr<Buffer> m_IndexBuffer;
		std::unique_ptr<RHIBuffer> m_IndexBuffer;
		uint32_t m_IndexCount;
		AABB m_AABB;

	};
	using MeshManager = ResourceManager<Mesh, std::string>;
}
