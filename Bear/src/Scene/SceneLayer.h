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

		static const std::vector<std::pair<const char*, const char*>>& GetModelList();

	private:
		std::unique_ptr<Scene> m_Scene;
	};
}
