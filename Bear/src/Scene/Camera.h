#pragma once
#include <glm/glm.hpp>

#include "Frustum.h"

namespace Bear
{
	// perspective or orthographic projection
	enum class ProjectionType {
		Perspective,
		Orthographic
	};
	
	class BEAR_API Camera
	{
	public:
		Camera(float fov, float aspectRatio, float nearPlane, float farPlane);
		virtual ~Camera() = default;

		void SetPerspective(float fov, float aspectRatio, float nearPlane, float farPlane);
		void SetOrthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane);

		void SetViewportSize(uint32_t width, uint32_t height);
		// matrices
		const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
		const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
		const glm::mat4& GetViewProjectionMatrix() const { return m_ViewProjectionMatrix; }
		// camera position and orientation
		glm::vec3 GetUpDirection() const;
		glm::vec3 GetRightDirection() const;
		glm::vec3 GetForwardDirection() const;

		const glm::vec3& GetPosition() const { return m_Position; }
		void SetPosition(const glm::vec3& position) { m_Position = position; RecalculateViewMatrix(); }
		// rotation in degrees
		float GetPitch() const { return m_Pitch; }
		float GetYaw() const { return m_Yaw; }
		void SetRotation(float pitch, float yaw) { m_Pitch = pitch; m_Yaw = yaw; RecalculateViewMatrix(); }

		// get frustum
		const Frustum& GetFrustum() const { return m_Frustum; }

	private:
		void RecalculateViewMatrix();
		void RecalculateProjectionMatrix();
		void CalculateForwardDirection();

	private:
		ProjectionType m_ProjectionType = ProjectionType::Perspective;

		glm::vec3 m_ForwardDirection;

		float m_PerspectiveFOV = glm::radians(45.0f);
		float m_PerspectiveNear = 0.1f;
		float m_PerspectiveFar = 1000.0f;

		float m_OrthographicSize = 10.0f;
		float m_OrthographicNear = -1.0f;
		float m_OrthographicFar = 1.0f;

		float m_AspectRatio = 1.778f; // 16:9

		// viewport
		float m_Pitch = 0.0f; // pitch
		float m_Yaw = 0.0f;   // yaw

		// cached matrices
		glm::mat4 m_ProjectionMatrix{ 1.0f };
		glm::mat4 m_ViewMatrix{ 1.0f };
		glm::mat4 m_ViewProjectionMatrix{ 1.0f };
		glm::vec3 m_Position{ 0.0f, 0.0f, 3.0f };

		// frustum
		Frustum m_Frustum;
	};
}
