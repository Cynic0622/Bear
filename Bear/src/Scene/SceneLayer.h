#pragma once
#include "Layer.h"

namespace Bear
{
	struct SceneData
	{
		glm::mat4 viewMaterix;
		glm::mat4 projectionMatrix;
	};
	class EditorCamera;
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

	private:
		std::unique_ptr<Scene> m_Scene;
		std::unique_ptr<EditorCamera> m_EditorCamera;
		SceneData m_SceneData;
	};
}
