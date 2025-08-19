#pragma once
#include "Layer.h"
#include "Texture.h"

namespace Bear
{
	struct LightData
	{
		glm::vec4 position; // Positions of lights.
		glm::vec4 colorIntensity; // xyz: color, w: intensity
	};
	struct SceneData
	{
		glm::mat4 viewMatrix;
		glm::mat4 projectionMatrix;

		glm::vec4 cameraPosition; // Camera position in world space.
		LightData lightsData[50];
		alignas(16) int lightCount = 50; // Number of lights in the scene.
	};
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
