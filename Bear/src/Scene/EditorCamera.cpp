#include "bearpch.h"
#include "EditorCamera.h"
#include "Events/Event.h"
namespace Bear
{
	EditorCamera::EditorCamera(float fov, float aspectRatio, float nearPlane, float farPlane)
		:Camera(fov, aspectRatio, nearPlane, farPlane)
	{
	}
	void EditorCamera::Update(float deltaTime)
	{
		if (Input::IsKeyPressed(Key::W))
		{
			m_CameraPosition += GetForwardDirection()  * deltaTime;
		}
		else if (Input::IsKeyPressed(Key::S))
		{
			m_CameraPosition -= GetForwardDirection() * deltaTime;
		}
		if (Input::IsKeyPressed(Key::A))
		{
			m_CameraPosition -= GetRightDirection() * deltaTime;
		}
		else if (Input::IsKeyPressed(Key::D))
		{
			m_CameraPosition += GetRightDirection() * deltaTime;
		}

		SetPosition(m_CameraPosition);
	}

	void EditorCamera::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowResizeEvent>([this](const WindowResizeEvent& event) { return this->OnWindowResize(event); });
	}
	bool EditorCamera::OnWindowResize(const WindowResizeEvent& event)
	{
		SetViewportSize(event.GetWidth(), event.GetHeight());
		BEAR_CORE_INFO("set camera viewport successfully.");
		return false; // Return false to propagate the event further
	}
}
