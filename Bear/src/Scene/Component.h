#pragma once
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "Entity.h"
namespace Bear
{
	using UID = uint32_t;
	struct IDComponent
	{
		UID ID;
		IDComponent() : ID(0) {}
		IDComponent(UID id) : ID(id) {}
	};

	struct TagComponent
	{
		std::string Tag;
		TagComponent() = default;
		TagComponent(std::string tag) : Tag(std::move(tag)) {}
		TagComponent(const TagComponent&) = default;
	};

	struct TransformComponent
	{
		glm::vec3 Position = glm::vec3(0.0f);
		glm::vec3 Scale = glm::vec3(1.0f);
		glm::vec3 Rotation = glm::vec3( 0.0f, 0.0f, 0.0f);
		glm::mat4 Transform = glm::mat4(1.0f);
		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const glm::vec3& position)
			: Position(position), Scale(glm::vec3(1.0f)), Rotation(glm::vec3(0.0f)) {
		}
		TransformComponent(const glm::vec3& position, const glm::vec3& scale, const glm::vec3& rotation)
			: Position(position), Scale(scale), Rotation(rotation) {
		}
		glm::mat4 GetTransform() const
		{
			return glm::translate(glm::mat4(1.0f), Position) * glm::toMat4(glm::quat(Rotation)) * glm::scale(glm::mat4(1.0f), Scale);
		}
	};

	struct MeshComponent
	{
		std::string MeshPath;
		std::shared_ptr<Mesh> MeshRes;
		MeshComponent() = default;
		MeshComponent(const std::string& meshPath)
			: MeshPath(meshPath), MeshRes(nullptr) {
		}
	};

	struct MaterialComponent
	{
		std::string MaterialPath;
		std::shared_ptr<Material> MaterialRes;
		MaterialComponent() = default;
		MaterialComponent(const std::string& materialPath)
			: MaterialPath(materialPath), MaterialRes(nullptr) {
		}
	};

	struct HierarchyComponent
	{
		Entity Parent;
		std::vector<Entity> Children;
	};
}
