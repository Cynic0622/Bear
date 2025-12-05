#pragma once
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "Entity.h"
#include "Material.h"

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
		glm::mat4 WorldTransform = glm::mat4(1.0f);
		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const glm::vec3& position)
			: Position(position), Scale(glm::vec3(1.0f)), Rotation(glm::vec3(0.0f)) {
		}
		TransformComponent(const glm::vec3& position, const glm::vec3& scale, const glm::vec3& rotation)
			: Position(position), Scale(scale), Rotation(rotation) {
		}
		TransformComponent(const glm::vec3& position, const glm::vec3& scale, const glm::quat& rotation)
			:Position(position), Scale(scale), Rotation(glm::eulerAngles(rotation))
		{
		}
		glm::mat4 GetLocalTransform() const
		{
			return glm::translate(glm::mat4(1.0f), Position) * glm::toMat4(glm::quat(Rotation)) * glm::scale(glm::mat4(1.0f), Scale);
		}
		glm::mat4 GetWorldTransform() const
		{
			return WorldTransform;
		}
		//void SetTransform(const glm::vec3& position, const glm::vec3& scale, const glm::quat& rotation)
		//	:Position(position), Scale(scale), Rotation(glm::eulerAngles(rotation))
		//{
		//	
		//}
		//void SetTransform(const glm::vec3& position, const glm::vec3& scale, const glm::vec3& rotation)
		//{
		//	Position = position;
		//	Scale = scale;
		//	Rotation = rotation;
		//	WorldTransform = GetLocalTransform();
		//}
	};

	struct MeshComponent
	{
		std::string MeshPath;
		std::shared_ptr<Mesh> MeshRes;
		//std::shared_ptr<Material> MaterialRes; // Optional material resource for the mesh
		MeshComponent() = default;
		MeshComponent(const std::string& meshPath)
			: MeshPath(meshPath), MeshRes(nullptr) {
		}
		MeshComponent(const std::shared_ptr<Mesh>& meshRes)
			: MeshPath(""), MeshRes(meshRes) {
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
		MaterialComponent(const std::shared_ptr<Material>& materialRes)
			: MaterialPath(""), MaterialRes(materialRes) {
		}
	};

	struct HierarchyComponent
	{
		//Entity Parent;
		// In components, use entt::entity to represent entities.
		entt::entity Parent = entt::null;
		//std::vector<Entity> Children;
		std::vector<entt::entity> Children;
	};

	struct LightComponent
	{
		glm::vec3 Color = glm::vec3(1.0f);
		float Intensity = 1.0f; // Light intensity
		LightComponent() = default;
		LightComponent(const glm::vec3& color, float intensity)
			: Color(color), Intensity(intensity) {
		}
	};
}
