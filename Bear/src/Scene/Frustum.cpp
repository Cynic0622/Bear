#include "bearpch.h"
#include "Frustum.h"

namespace Bear
{
	bool Frustum::Contains(AABB aabb) const
	{
		for (int i = 0; i < 6; ++i)
		{
			const Plane& plane = m_Planes[i];
			glm::vec3 positiveVertex = aabb.GetPositiveVertex(plane.normal);
			if (plane.GetDistance(positiveVertex) < 0.0f)
			{
				return false;
			}
		}
		return true;
	}
	Frustum Frustum::CreateFromMatrix(const glm::mat4& viewProjectionMatrix)
	{
		Frustum frustum;

		const glm::mat4& m = viewProjectionMatrix;

		// Left plane
		frustum.m_Planes[Left] = Plane(glm::vec3(m[0][3] + m[0][0], m[1][3] + m[1][0], m[2][3] + m[2][0]), m[3][3] + m[3][0]);
		// Right plane
		frustum.m_Planes[Right] = Plane(glm::vec3(m[0][3] - m[0][0], m[1][3] - m[1][0], m[2][3] - m[2][0]), m[3][3] - m[3][0]);
		// Bottom plane
		frustum.m_Planes[Bottom] = Plane(glm::vec3(m[0][3] + m[0][1], m[1][3] + m[1][1], m[2][3] + m[2][1]), m[3][3] + m[3][1]);
		// Top plane
		frustum.m_Planes[Top] = Plane(glm::vec3(m[0][3] - m[0][1], m[1][3] - m[1][1], m[2][3] - m[2][1]), m[3][3] - m[3][1]);
		// Near plane
		frustum.m_Planes[Near] = Plane(glm::vec3(m[0][3] + m[0][2], m[1][3] + m[1][2], m[2][3] + m[2][2]), m[3][3] + m[3][2]);
		// Far plane
		frustum.m_Planes[Far] = Plane(glm::vec3(m[0][3] - m[0][2], m[1][3] - m[1][2], m[2][3] - m[2][2]), m[3][3] - m[3][2]);

		return frustum;
	}
}
