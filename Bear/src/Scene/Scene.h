#pragma once
#include <entt/entt.hpp>

namespace Bear
{
	struct Resources;
	class Node;
	struct RenderObject;
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
		void Update(float deltaTime);

		void CollectRenderObjects(std::vector<RenderObject>& ObjectsList, SceneData& sceneData);

		CameraController* GetCamera() const { return m_GameCamera.get(); }
		// entity management
		Entity CreateEntity(const std::string& name = "Entity");
		void DestroyEntity(Entity entity);

		void CreateSceneGraph(const ModelDescription& desc, const Resources& resources);


		// deal with events
		void OnEvent(Event& event);
		bool OnKeyPress(Event& event);

	private:
		//std::unique_ptr<Node> m_RootNode; // the root node of the scene graph
		friend class Entity; // allow Entity class to access registry.
		friend class SceneLayer;
		entt::registry m_Registry; // the registry for the scene, used for storing entities and components
		// std::unique_ptr<MeshManager> m_MeshManager;
		// std::unique_ptr<MaterialManager> m_MaterialManager;
		// std::unique_ptr<TextureManager> m_TextureManager;
		std::unique_ptr<CameraController> m_GameCamera; // scene game mode camera.
		std::unique_ptr<CameraController> m_EditorCamera; // scene editor mode camera.
		bool m_EditorMode = true; // true: editor mode, false: game mode
		bool m_FrustumCull = true; // true: frustum culling enabled, false: disabled

	private:

		CameraController* GetActiveCamera() const;
	};
}
