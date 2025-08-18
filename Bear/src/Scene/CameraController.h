#pragma once

#include "Camera.h"

namespace Bear
{
	class Event;

	class BEAR_API CameraController : public Camera
	{
	public:
		CameraController(float fov, float aspectRatio, float nearPlane, float farPlane);

		void Update(float deltaTime);
		void OnEvent(Event& event);

	private:
		glm::vec3 m_CameraPosition = { 0.0f, 0.0f, 3.0f };
		glm::vec2 m_LastMousePosition = { 640.0f, 360.0f };
		float m_CameraPitch = 0.f, m_CameraYaw = 0.0f;

		bool m_IsMousePressed = false; // determine if ui has captured mouse input.
	private:
		bool OnWindowResize(const WindowResizeEvent& event);
		bool OnMouseScroll(const MouseScrolledEvent& event);
		bool OnMousePress(const MouseButtonPressedEvent& event);
		bool OnMouseRelease(const MouseButtonReleasedEvent& event);
	};
}
