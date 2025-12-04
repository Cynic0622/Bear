#pragma once
#include <glm/glm.hpp>
namespace Bear
{
	struct Light
	{
		glm::vec4 color = glm::vec4(1.f); // xyz: color, w: intensity
	};
	struct PointLight : Light
	{
		glm::vec4 position = glm::vec4(0.f, 0.f, 0.f, 1.f); // Positions of lights.
	};
	struct DirectionalLight : Light
	{
		glm::vec4 direction = glm::vec4(0.f, -1.f, 0.f, 0.f); // Direction of the light.
	};
}
