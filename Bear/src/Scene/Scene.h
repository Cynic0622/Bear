#pragma once
#include <entt/entt.hpp>

#include "Material.h"
#include "Common/Texture.h"
#include "Common/Mesh.h"


namespace Bear
{
	class Node;
	class RenderObject;
	class Entity;
	struct ModelDescription;

	class BEAR_API Scene
	{
	public:
		Scene();
		~Scene() = default;

		// update the all the transforms in the scene
		void Update();

		void CollectRenderObjects(std::vector<RenderObject>& ObjectsList);

		Node* GetRootNode() const { return m_RootNode.get(); }
		MeshManager* GetMeshManager() const { return m_MeshManager.get(); }
		MaterialManager* GetMaterialManager() const { return m_MaterialManager.get(); }
		TextureManager* GetTextureManager() const { return m_TextureManager.get(); }
		// entity management
		Entity CreateEntity(const std::string& name = "Entity");
		void DestroyEntity(Entity entity);

	private:
		std::unique_ptr<Node> m_RootNode; // the root node of the scene graph
		friend class Entity; // allow Entity class to access registry.
		friend class SceneLayer;
		entt::registry m_Registry; // the registry for the scene, used for storing entities and components
		std::unique_ptr<MeshManager> m_MeshManager;
		std::unique_ptr<MaterialManager> m_MaterialManager;
		std::unique_ptr<TextureManager> m_TextureManager;

	private:
		void CollectRenderObjectsRecursive(const Node* node, std::vector<RenderObject>& renderList);

		void InstantiateModel(const ModelDescription& description);
	};
}
