#pragma once
#include "KeyCode.h"
#include "MouseCode.h"
struct GLFWwindow;
namespace Bear
{
	class Input
	{
	public:
		static void Init(GLFWwindow* window);
		static bool IsKeyPressed(KeyCode key);

		static bool IsMouseButtonPressed(MouseCode button);

		static float GetMouseX();
		static float GetMouseY();
		static glm::vec2 GetMousePosition();

	protected:
		static GLFWwindow* window;
	};
}
