#include "bearpch.h"

#include "Input.h"

namespace Bear
{
	GLFWwindow* Input::window = nullptr;
	void Input::Init(GLFWwindow* window_)
	{
		window = window_;
	}
	bool Input::IsKeyPressed(KeyCode key)
	{
		//window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		auto state = glfwGetKey(window, key);
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}
	bool Input::IsMouseButtonPressed(MouseCode button)
	{
		auto state = glfwGetMouseButton(window, button);
		return state == GLFW_PRESS;
	}
	float Input::GetMouseX()
	{
		return GetMousePosition().x;
	}
	float Input::GetMouseY()
	{
		return GetMousePosition().y;
	}
	glm::vec2 Input::GetMousePosition()
	{
		double x, y;
		glfwGetCursorPos(window, &x, &y);
		return glm::vec2(x, y);
	}
}
