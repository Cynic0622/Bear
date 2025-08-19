#pragma once
namespace Bear
{
	struct AABB
	{
		glm::vec3 min = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 max = glm::vec3(0.0f, 0.0f, 0.0f);

		AABB() = default;
		AABB(const glm::vec3& minPoint, const glm::vec3& maxPoint)
			: min(minPoint), max(maxPoint) {
		}

		glm::vec3 GetCenter() const { return (min + max) * 0.5f; }
		glm::vec3 GetSize() const { return max - min; }
		glm::vec3 GetHalfSize() const { return (max - min) * 0.5f; }

		glm::vec3 GetPositiveVertex(const glm::vec3& normal) const
		{
			glm::vec3 positiveVertex = min;
			if (normal.x > 0) positiveVertex.x = max.x;
			if (normal.y > 0) positiveVertex.y = max.y;
			if (normal.z > 0) positiveVertex.z = max.z;
			return positiveVertex;
		}
	};

	struct Plane
	{
		glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
		float distance = 0.0f;

		Plane() = default;
		Plane(const glm::vec3& normal, float distance) : normal(normal), distance(distance)
		{
			float len = glm::length(this->normal);
			if (len > 0)
			{
				this->normal /= len;
				this->distance /= len;
			}
		}
		float GetDistance(const glm::vec3& point) const
		{
			return glm::dot(normal, point) + distance;
		}
	};
	class Frustum
	{
	public:
		enum FrustumPlane : uint8_t {
			Left = 0,
			Right,
			Bottom,
			Top,
			Near,
			Far
		};
	
		Frustum() = default;
		~Frustum() = default;

		bool Contains(AABB aabb) const;

		static Frustum CreateFromMatrix(const glm::mat4& viewProjectionMatrix);
	private:
		std::array<Plane, 6> m_Planes;
	};
}
