#pragma once

#include "Camera.h"

namespace Bear
{
	class Event;

	class BEAR_API EditorCamera : public Camera
	{
	public:
		EditorCamera(float fov, float aspectRatio, float nearPlane, float farPlane);

		void Update(float deltaTime);
		void OnEvent(Event& event);

	private:
		glm::vec3 m_CameraPosition = { 0.0f, 0.0f, 3.0f };
	private:
		bool OnWindowResize(const WindowResizeEvent& event);
	};
}
