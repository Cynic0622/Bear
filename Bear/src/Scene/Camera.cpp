#include "bearpch.h"
#include "Camera.h"

namespace Bear
{
	Camera::Camera(float fov, float aspectRatio, float nearPlane, float farPlane)
	{
		m_PerspectiveFOV = fov;
		m_AspectRatio = aspectRatio;
		m_PerspectiveNear = nearPlane;
		m_PerspectiveFar = farPlane;
		m_ProjectionType = ProjectionType::Perspective;
		RecalculateProjectionMatrix();
		RecalculateViewMatrix();
		CalculateForwardDirection();
	}
	void Camera::SetPerspective(float fov, float aspectRatio, float nearPlane, float farPlane)
	{
		m_PerspectiveFOV = fov;
		m_AspectRatio = aspectRatio;
		m_PerspectiveNear = nearPlane;
		m_PerspectiveFar = farPlane;
		RecalculateProjectionMatrix();
		RecalculateViewMatrix();
	}
	void Camera::SetOrthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane)
	{
	}
	void Camera::SetViewportSize(uint32_t width, uint32_t height)
	{
		if (width > 0 && height > 0) {
			m_AspectRatio = (float)width / (float)height;
			RecalculateProjectionMatrix();
		}
	}
	glm::vec3 Camera::GetUpDirection() const
	{
		return glm::normalize(glm::cross(GetRightDirection(), GetForwardDirection()));
	}
	glm::vec3 Camera::GetRightDirection() const
	{
		return glm::normalize(glm::cross(GetForwardDirection(), glm::vec3(0.f, 1.f, 0.f)));
	}
	glm::vec3 Camera::GetForwardDirection() const
	{
		return m_ForwardDirection;
	}
	void Camera::RecalculateViewMatrix()
	{
		m_ViewMatrix = glm::lookAt(m_Position, GetForwardDirection() + m_Position, GetUpDirection());
	}
	void Camera::RecalculateProjectionMatrix()
	{
		if (m_ProjectionType == ProjectionType::Perspective)
		{
			m_ProjectionMatrix = glm::perspective(glm::radians(m_PerspectiveFOV), m_AspectRatio, m_PerspectiveNear, m_PerspectiveFar);
		}

		m_ProjectionMatrix[1][1] *= -1.0f; // Invert Y-axis for Vulkan compatibility
	}
	void Camera::CalculateForwardDirection()
	{
		m_ForwardDirection =  glm::normalize(glm::vec3(cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch)),
			sin(glm::radians(m_Pitch)),
			sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch))));
	}
	/*void Camera::SetOrthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane)
	{
		m_OrthographicLeft = left;
		m_OrthographicRight = right;
		m_OrthographicBottom = bottom;
		m_OrthographicTop = top;
		m_OrthographicNear = nearPlane;
		m_OrthographicFar = farPlane;
		RecalculateProjectionMatrix();
	}*/
}
