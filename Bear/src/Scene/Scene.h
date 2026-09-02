#pragma once
#include <entt/entt.hpp>
#include "BaseData.h"
namespace Bear
{
	struct Resources;
	class Node;
	struct RenderObject;
	class Entity;
	struct ModelDescription;
	class CameraController;

	class BEAR_API Scene
	{
	public:
		Scene();
		~Scene();

		// update the all the transforms in the scene
		void Update(float deltaTime);

		void CollectRenderData(std::vector<RenderObject>& ObjectsList, SceneData& sceneData, BaseData& baseData);

		CameraController* GetCamera() const { return m_GameCamera.get(); }
		// entity management
		Entity CreateEntity(const std::string& name = "Entity");
		void DestroyEntity(Entity entity);

		void AddLight(const PointLight& light);

		void CreateSceneGraph(const ModelDescription& desc, const Resources& resources);
		void ClearAllEntities();
		void MultiplyInstances(uint32_t count, float spread);


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
		int m_LightNumber = 0;
		uint32_t m_FrameNum = 0;
		std::atomic<uint32_t> m_NextUID{ 1 }; // for generating unique entity IDs and use in the future.
		std::vector<entt::entity> m_UpdateOrder; // parent-before-child order for transform propagation
		bool m_UpdateOrderDirty = true;
		uint32_t m_TotalMeshEntities = 0;    // mesh entities before culling (updated in CollectRenderData)
		uint32_t m_VisibleMeshEntities = 0;  // after frustum culling

	private:

		CameraController* GetActiveCamera() const;
		uint32_t GetTotalMeshEntities() const { return m_TotalMeshEntities; }
		uint32_t GetVisibleMeshEntities() const { return m_VisibleMeshEntities; }
		bool IsFrustumCullingEnabled() const { return m_FrustumCull; }
		bool IsEditorMode() const { return m_EditorMode; }
	};
}
