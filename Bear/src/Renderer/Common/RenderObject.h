#pragma once

#include <glm/glm.hpp>
#include "Mesh.h"
#include "Material.h"
#include <memory>
#include <glm/gtc/matrix_transform.hpp>

namespace Bear {

	class VulkanDevice;
	class Mesh;
	class Material;
	struct TransformComponent
	{
		glm::vec3 translation{ 0.0f, 0.0f, 0.0f };
		glm::vec3 rotation{ 0.0f, 0.0f, 0.0f };
		glm::vec3 scale{ 1.0f, 1.0f, 1.0f };

		glm::mat4 GetTransform() const
		{
			glm::mat4 mat = glm::translate(glm::mat4(1.0f), translation);
			mat = glm::rotate(mat, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			mat = glm::rotate(mat, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
			mat = glm::rotate(mat, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
			mat = glm::scale(mat, scale);
			return mat;
		}
	};

	class RenderObject
	{
	public:
		// 使用共享指针，这使得我们可以只创建一个重量级的 Mesh（比如一个复杂的模型）和一个 Material（比如“金属材质”），
		// 然后创建成百上千个 RenderObject 实例来共享它们，极大地节省了内存
		std::shared_ptr<Mesh> mesh;
		std::shared_ptr<Material> material;
		TransformComponent transform;

		static std::unique_ptr<RenderObject> Create(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Material>& material)
		{
			auto obj = std::make_unique<RenderObject>();
			obj->mesh = mesh;
			obj->material = material;
			return obj;
		}
	};
}