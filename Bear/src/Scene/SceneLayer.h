#pragma once
#include "Layer.h"
#include "Texture.h"
#include <vector>
#include <string>
namespace Bear
{
	class CameraController;
	class Renderer;
	class Scene;
	
	class BEAR_API SceneLayer : public Layer
	{
	public:
		SceneLayer(std::unique_ptr<Scene> scene);
		~SceneLayer() override;

		SceneLayer(const SceneLayer&) = delete;
		SceneLayer& operator=(const SceneLayer&) = delete;

		void OnAttach() override;
		void OnDetach() override;

		void OnUpdate(float deltaTime) override;
		void OnRender() const override;

		void OnEvent(Event& event) override;
		bool OnKeyPress(Event& event);

		void LoadModel(const std::string& path);
		void ReloadModel();
		void SetInstanceMultiplier(uint32_t count);
		uint32_t GetInstanceMultiplier() const { return m_InstanceMultiplier; }

		static const std::vector<std::pair<const char*, const char*>>& GetModelList();

		Scene* GetScene() const;
		uint32_t GetTotalMeshEntities() const;
		uint32_t GetVisibleMeshEntities() const;
		bool IsFrustumCullingEnabled() const;
		bool IsEditorMode() const;

	private:
		std::unique_ptr<Scene> m_Scene;
		std::string m_CurrentModelPath;
		uint32_t m_InstanceMultiplier = 1;
	};
}
