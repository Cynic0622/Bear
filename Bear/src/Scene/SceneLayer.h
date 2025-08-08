#pragma once
#include "Layer.h"
namespace Bear
{
	class Renderer;
	class Scene;
	class BEAR_API SceneLayer : public Layer
	{
	public:
		SceneLayer(std::unique_ptr<Scene> scene);
		~SceneLayer() override = default;

		SceneLayer(const SceneLayer&) = delete;
		SceneLayer& operator=(const SceneLayer&) = delete;

		void OnAttach() override;
		void OnDetach() override;

		void OnUpdate(float deltaTime) override;
		void OnRender() const override;

	private:
		std::unique_ptr<Scene> m_Scene;
	};
}
