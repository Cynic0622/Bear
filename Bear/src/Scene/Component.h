#pragma once
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "Entity.h"
#include "Material.h"
#include "Frustum.h"

namespace Bear
{
	using UID = uint32_t;
	struct IDComponent
	{
		UID ID = 0; // 0 is invalid ID.
		IDComponent() = default;
		explicit IDComponent(UID id) : ID(id) {}
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
		TransformComponent() = default;
		TransformComponent(const glm::vec3& position)
			: m_Position(position) {
		}
		TransformComponent(const glm::vec3& position, const glm::vec3& scale, const glm::vec3& rotation)
			: m_Position(position), m_Scale(scale), m_Rotation(rotation) {
		}
		TransformComponent(const glm::vec3& position, const glm::vec3& scale, const glm::quat& rotation)
			: m_Position(position), m_Scale(scale), m_Rotation(glm::eulerAngles(rotation))
		{
		}
		TransformComponent(const TransformComponent& other)
			: m_Position(other.m_Position), m_Scale(other.m_Scale), m_Rotation(other.m_Rotation)
		{
			m_LocalDirty = true;
		}
		TransformComponent& operator=(const TransformComponent& other)
		{
			m_Position = other.m_Position;
			m_Scale = other.m_Scale;
			m_Rotation = other.m_Rotation;
			m_LocalDirty = true;
			return *this;
		}

		void SetPosition(const glm::vec3& position) { m_Position = position; m_LocalDirty = true; }
		void SetScale(const glm::vec3& scale) { m_Scale = scale; m_LocalDirty = true; }
		void SetRotation(const glm::vec3& rotation) { m_Rotation = rotation; m_LocalDirty = true; }
		void SetTransform(const glm::vec3& position, const glm::vec3& scale, const glm::vec3& rotation)
		{
			m_Position = position;
			m_Scale = scale;
			m_Rotation = rotation;
			m_LocalDirty = true;
		}

		const glm::vec3& GetPosition() const { return m_Position; }
		const glm::vec3& GetScale() const { return m_Scale; }
		const glm::vec3& GetRotation() const { return m_Rotation; }

		glm::mat4 GetLocalTransform() const
		{
			return glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(glm::quat(m_Rotation)) * glm::scale(glm::mat4(1.0f), m_Scale);
		}
		const glm::mat4& GetWorldTransform() const { return WorldTransform; }

		bool IsLocalDirty() const { return m_LocalDirty; }
		void ClearLocalDirty() { m_LocalDirty = false; }
		void MarkLocalDirty() { m_LocalDirty = true; }

		// world-space state, refreshed by Scene::Update (dirty propagation)
		glm::mat4 WorldTransform = glm::mat4(1.0f);
		bool WorldDirty = false;   // transient per-frame flag, valid only during Scene::Update
		AABB WorldAABB;              // cached world-space AABB (only valid when WorldAABBDirty == false)
		bool WorldAABBDirty = true;

	private:
		glm::vec3 m_Position = glm::vec3(0.0f);
		glm::vec3 m_Scale = glm::vec3(1.0f);
		glm::vec3 m_Rotation = glm::vec3(0.0f, 0.0f, 0.0f);
		bool m_LocalDirty = true;
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
		entt::entity Parent = entt::null;
		std::vector<entt::entity> Children;
	};

	enum LightType : uint8_t
	{
		Directional = 0,
		Point = 1,
		// TODO: implement spotlight and area light.
		Spot = 2,
		Area = 3
	};
	struct LightComponent
	{
		LightType Type = LightType::Point;
		glm::vec4 Color = glm::vec4(1.f); // w for intensity.

		// directional light.
		glm::vec3 Direction = glm::vec3(0.f, -1.f, 0.f);

		LightComponent() = default;
		LightComponent(const glm::vec3& color, float intensity)
		{
			Color = glm::vec4(color, intensity);
		}
		LightComponent(LightType type, const glm::vec4& color)
			: Type(type), Color(color) {}

		static LightComponent CreateDirectional(const glm::vec3& direction, const glm::vec4& color)
		{
			LightComponent light(LightType::Directional, color);
			light.Direction = direction;
			return light;
		}
		static LightComponent CreatePoint(glm::vec4 color)
		{
			return { LightType::Point, color };
		}
	};
}
