#pragma once
#include "Layer.h"
#include "Texture.h"
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

	private:
		std::unique_ptr<Scene> m_Scene;
	};
}
