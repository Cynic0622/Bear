#pragma once
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

		void Draw(RHICommandList& cmd) const;
		void SetupIndirect(int32_t globalVertexOffset, int32_t globalFirstIndex);

		// Getters
		const AABB& GetAABB() const { return m_AABB; }
		uint32_t GetIndexCount() const { return m_IndexCount; }
		int32_t GetGlobalVertexOffset() const { return m_GlobalVertexOffset; }
		uint32_t GetGlobalFirstIndex() const { return m_GlobalFirstIndex; }

		static void SetGlobalBuffers(RHIBuffer* vertexBuffer, RHIBuffer* indexBuffer);
		static RHIBuffer* GetGlobalVertexBuffer() { return s_GlobalVertexBuffer; }
		static RHIBuffer* GetGlobalIndexBuffer() { return s_GlobalIndexBuffer; }

	private:
		std::unique_ptr<RHIBuffer> m_VertexBuffer;
		std::unique_ptr<RHIBuffer> m_IndexBuffer;
		uint32_t m_IndexCount;
		AABB m_AABB;

		int32_t m_GlobalVertexOffset = -1;
		uint32_t m_GlobalFirstIndex = 0;

		static RHIBuffer* s_GlobalVertexBuffer;
		static RHIBuffer* s_GlobalIndexBuffer;

	};
}
