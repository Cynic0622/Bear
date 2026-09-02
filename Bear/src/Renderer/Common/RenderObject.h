#pragma once

#include <glm/glm.hpp>
#include "Mesh.h"
#include "Material.h"
#include <memory>
#include <glm/gtc/matrix_transform.hpp>

namespace Bear {

	class Device;
	class Mesh;
	class Material;

	struct RenderObject
	{
		// use shared_ptr to allow multiple RenderObjects to share the same Mesh and Material, reducing memory usage
		std::shared_ptr<Mesh> mesh;
		std::shared_ptr<Material> material;
		glm::mat4 transform {1.0f};
		mutable AABB m_CachedAABB; // cache the world AABB for this object
		mutable bool m_AABBDirty = true;
		RenderObject() = default;

		const AABB& GetAABB () const
		{
			if (m_AABBDirty && mesh)
			{
				m_CachedAABB = TransformAABB(mesh->GetAABB(), transform);
				m_AABBDirty = false;
			}
			return m_CachedAABB;
		}
	private:
	};
}