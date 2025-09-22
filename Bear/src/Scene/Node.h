#pragma once
#include <glm/gtc/quaternion.hpp>
#include "Common/Mesh.h"
#include "Common/Material.h"
namespace Bear
{
	class Material;
	class Mesh;

	class Node
	{
	public:
		Node(std::string name = "Node");
		virtual ~Node() = default;

		Node(const Node&) = delete;
		Node& operator=(const Node&) = delete;
		// allow move semantics
		Node(Node&&) = default;
		Node& operator=(Node&&) = default;

		// transform methods
		void SetPosition(const glm::vec3& position);
		void SetRotation(const glm::quat& rotation);
		void SetScale(const glm::vec3& scale);

		// getters
		const std::string& GetName() const { return m_Name; }
		const glm::vec3& GetPosition() const { return m_Position; }
		const glm::quat& GetRotation() const { return m_Rotation; }
		const glm::vec3& GetScale() const { return m_Scale; }

		// scene layer
		void AddChild(std::unique_ptr<Node> child);
		Node* GetParent() const { return m_Parent; }
		const std::vector<std::unique_ptr<Node>>& GetChildren() const { return m_Children; }
		// update method for scene graph traversal
		void Update(bool forceUpdate = false);
		const glm::mat4& GetWorldTransform() const { return m_WorldTransform; }

		// additional methods for scene management
		void SetMesh(const std::shared_ptr<Mesh>& mesh) { m_Mesh = mesh; }
		void SetMaterial(const std::shared_ptr<Material> material) { m_Material = material; }
		std::shared_ptr<Mesh> GetMesh() const { return m_Mesh; }
		std::shared_ptr<Material> GetMaterial() const { return m_Material; }
	private:
		std::string m_Name;
		glm::vec3 m_Position{ 0.0f, 0.0f, 0.0f };
		glm::quat m_Rotation{ 1.0f, 0.0f, 0.0f, 0.0f }; // quaternion for rotation w, x, y, z
		glm::vec3 m_Scale{ 1.0f, 1.0f, 1.0f };
		Node* m_Parent{ nullptr };
		std::vector<std::unique_ptr<Node>> m_Children;
		glm::mat4 m_WorldTransform{ 1.0f };
		std::shared_ptr<Mesh> m_Mesh{ nullptr };
		std::shared_ptr<Material> m_Material{ nullptr };

		bool m_IsDirty = true;

	private:
		void UpdateWorldTransform();
		void SetDirty();
	};
}
