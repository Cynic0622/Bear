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
		static AABB TransformAABB(const AABB& localAABB, const glm::mat4& transform)
        {
            if (localAABB.min == localAABB.max)
                return localAABB;

            glm::vec3 center = localAABB.GetCenter();
            glm::vec3 worldCenter = glm::vec3(transform * glm::vec4(center, 1.0f));
            
			glm::mat3 mat = glm::mat3(transform);
			glm::mat3 absTransform;
			absTransform[0] = glm::abs(mat[0]);
			absTransform[1] = glm::abs(mat[1]);
			absTransform[2] = glm::abs(mat[2]);
            glm::vec3 halfSize = localAABB.GetHalfSize();
            glm::vec3 newHalfSize = absTransform * halfSize;
            
            return AABB(worldCenter - newHalfSize, worldCenter + newHalfSize);
        }
	};
}