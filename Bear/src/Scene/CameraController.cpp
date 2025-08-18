#include "bearpch.h"
#include "CameraController.h"
#include "Events/Event.h"
namespace Bear
{
	CameraController::CameraController(float fov, float aspectRatio, float nearPlane, float farPlane)
		:Camera(fov, aspectRatio, nearPlane, farPlane)
	{
	}
	void CameraController::Update(float deltaTime)
	{
		float speed = 100.f;
		if (Input::IsKeyPressed(Key::W))
		{
			m_CameraPosition += GetForwardDirection()  * deltaTime * speed;
		}
		else if (Input::IsKeyPressed(Key::S))
		{
			m_CameraPosition -= GetForwardDirection() * deltaTime * speed;
		}
		if (Input::IsKeyPressed(Key::A))
		{
			m_CameraPosition -= GetRightDirection() * deltaTime * speed;
		}
		else if (Input::IsKeyPressed(Key::D))
		{
			m_CameraPosition += GetRightDirection() * deltaTime * speed;
		}
		auto mousePos = Input::GetMousePosition();
		if ( m_IsMousePressed && Input::IsMouseButtonPressed(Mouse::Left))
		{
			glm::vec2 delta = mousePos - m_LastMousePosition;
			m_CameraPitch = std::clamp(m_CameraPitch + delta.y * 0.1f, -89.0f, 89.0f); // Clamp pitch to avoid gimbal lock
			m_CameraYaw += delta.x * 0.1f;
			SetRotation(m_CameraPitch, m_CameraYaw);
		}
		m_LastMousePosition = mousePos; // Update mouse positon every frame.
		SetPosition(m_CameraPosition);
	}

	void CameraController::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowResizeEvent>([this](const WindowResizeEvent& event) { return this->OnWindowResize(event); });
		dispatcher.Dispatch<MouseScrolledEvent>([this](const MouseScrolledEvent& event) { return this->OnMouseScroll(event); });
		dispatcher.Dispatch<MouseButtonPressedEvent>([this](const MouseButtonPressedEvent& event) { return this->OnMousePress(event); });
		dispatcher.Dispatch<MouseButtonReleasedEvent>([this](const MouseButtonReleasedEvent& event) { return this->OnMouseRelease(event); });
		if (event.IsHandled()) return;
	}
	bool CameraController::OnWindowResize(const WindowResizeEvent& event)
	{
		SetViewportSize(event.GetWidth(), event.GetHeight());
		BEAR_CORE_INFO("set camera viewport successfully.");
		return false; // Return false to propagate the event further
	}
	bool CameraController::OnMouseScroll(const MouseScrolledEvent& event)
	{
		float zoomAmount = event.GetYOffset() * 10.f; // Adjust zoom sensitivity as needed
		m_CameraPosition += GetForwardDirection() * zoomAmount;
		SetPosition(m_CameraPosition); // Update camera position
		return true;
	}
	bool CameraController::OnMousePress(const MouseButtonPressedEvent& event)
	{
		return m_IsMousePressed = true; // Update mouse pressed state.
	}
	bool CameraController::OnMouseRelease(const MouseButtonReleasedEvent& event)
	{
		return m_IsMousePressed = false; // Update mouse pressed state.
	}
}
