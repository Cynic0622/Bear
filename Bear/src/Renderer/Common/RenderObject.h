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
	/*struct TransformComponent
	{
		glm::vec3 translation{ 0.0f, 0.0f, 0.0f };
		glm::vec3 rotation{ 0.0f, 0.0f, 0.0f };
		glm::vec3 scale{ 1.0f, 1.0f, 1.0f };
		glm::mat4 transform{ 1.0f };

		glm::mat4 GetTransform() const
		{
			return transform;
		}
		void SetTransform(const glm::mat4& trans)
		{
			transform = trans;
		}
	};*/

	class RenderObject
	{
	public:
		// use shared_ptr to allow multiple RenderObjects to share the same Mesh and Material, reducing memory usage
		std::shared_ptr<Mesh> mesh;
		std::shared_ptr<Material> material;
		glm::mat4 transform = glm::mat4(1.0f); // Default to identity matrix
		//TransformComponent transformComponent;
		static std::unique_ptr<RenderObject> Create(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Material>& material, glm::mat4 transform)
		{
			auto obj = std::make_unique<RenderObject>();
			obj->mesh = mesh;
			obj->material = material;
			obj->transform = transform;
			return obj;
		}
	};
}