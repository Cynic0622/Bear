#pragma once
#include <entt/entt.hpp>

namespace Bear
{
	struct Resources;
	class Node;
	class RenderObject;
	class Entity;
	struct ModelDescription;
	class CameraController;
	struct SceneData;


	class BEAR_API Scene
	{
	public:
		Scene();
		~Scene();

		// update the all the transforms in the scene
		void Update(bool editorMode, float deltaTime);

		void CollectRenderObjects(std::vector<RenderObject>& ObjectsList, SceneData& sceneData);

		//Node* GetRootNode() const { return m_RootNode.get(); }
		// MeshManager* GetMeshManager() const { return m_MeshManager.get(); }
		// MaterialManager* GetMaterialManager() const { return m_MaterialManager.get(); }
		// TextureManager* GetTextureManager() const { return m_TextureManager.get(); }
		CameraController* GetCamera() const { return m_Camera.get(); }
		// entity management
		Entity CreateEntity(const std::string& name = "Entity");
		void DestroyEntity(Entity entity);

		void CreateSceneGraph(const ModelDescription& desc, const Resources& resources);

		void OnEvent(Event& event);

	private:
		//std::unique_ptr<Node> m_RootNode; // the root node of the scene graph
		friend class Entity; // allow Entity class to access registry.
		friend class SceneLayer;
		entt::registry m_Registry; // the registry for the scene, used for storing entities and components
		// std::unique_ptr<MeshManager> m_MeshManager;
		// std::unique_ptr<MaterialManager> m_MaterialManager;
		// std::unique_ptr<TextureManager> m_TextureManager;
		std::unique_ptr<CameraController> m_Camera; // scene camera.

	private:
		void CollectRenderObjectsRecursive(std::vector<RenderObject>& renderList);

		void InstantiateModel(const ModelDescription& description);
	};
}
