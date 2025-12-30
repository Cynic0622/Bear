#include "bearpch.h"
#include "WindowsWindow.h"
#include "Bear/Core.h"
#include "BearAssert.h"
#include "Bear/Events/ApplicationEvent.h"
#include "Bear/Events/KeyEvent.h"
#include "Bear/Events/MouseEvent.h"

#include <glad/glad.h>

namespace Bear {
	static bool s_GLFWInitialized = false;

	Window* Window::Create(const WindowProps& props)
	{
		return new WindowsWindow(props);
	}

	void WindowsWindow::WindowSizeCallback(GLFWwindow* window, int width, int height)
	{
		WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
		data.Width = width; // 更新宽度
		data.Height = height; // 更新高度
		//BEAR_CLIENT_INFO("Window resized to {0}, {1}", width, height);
		
		// 触发窗口大小改变事件
		WindowResizeEvent event(width, height);
		data.EventCallback(event);
	}

	void WindowsWindow::WindowCloseCallback(GLFWwindow* window)
	{
		WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
		//BEAR_CLIENT_INFO("Window closed: {0}", data.Title);
		
		// 触发窗口关闭事件
		WindowCloseEvent event;
		data.EventCallback(event);
		
		// 关闭窗口
		glfwSetWindowShouldClose(window, true);
	}

	void WindowsWindow::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

		// 处理按键事件，创建KeyEvent对象并调用回调函数
		switch (action) {
		case GLFW_PRESS: {
			//BEAR_CLIENT_INFO("Key pressed: {0}", key);
			KeyPressedEvent event(key, false);
			data.EventCallback(event); // 调用事件回调函数处理按键事件
			break;
		}
		case GLFW_RELEASE: {
			//BEAR_CLIENT_INFO("Key released: {0}", key);
			KeyReleasedEvent event(key);
			data.EventCallback(event);
			break;
		}
		case GLFW_REPEAT: {
			//BEAR_CLIENT_INFO("Key repeated: {0}", key);
			KeyPressedEvent event(key, true); // 重复按键事件，设置isRepeat为true
			data.EventCallback(event);
			break;
		}
		default:
			//BEAR_CLIENT_ERROR("Unknown key action: {0}", action);
			return; // 不处理未知动作
		};
	}

	void WindowsWindow::MouseMoveCallback(GLFWwindow* window, double xpos, double ypos)
	{
		WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
		//BEAR_CLIENT_INFO("Mouse moved to ({0}, {1})", xpos, ypos);
		
		// 触发鼠标移动事件
		MouseMovedEvent event((float)xpos, (float)ypos);
		data.EventCallback(event);
	}

	void WindowsWindow::MouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
	{
		WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
		//BEAR_CLIENT_INFO("Mouse scrolled by ({0}, {1})", xoffset, yoffset);
		
		// 触发鼠标滚轮事件
		MouseScrolledEvent event((float)xoffset, (float)yoffset);
		data.EventCallback(event);
	}

	void WindowsWindow::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
	{
		WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
		switch (action) {
		case GLFW_PRESS: {
			//BEAR_CLIENT_INFO("Mouse button pressed: {0}", button);
			MouseButtonPressedEvent event(button);
			data.EventCallback(event); // 调用事件回调函数处理鼠标按键事件
			break;
		}
		case GLFW_RELEASE: {
			//BEAR_CLIENT_INFO("Mouse button released: {0}", button);
			MouseButtonReleasedEvent event(button);
			data.EventCallback(event);
			break;
		}
		default:
			//BEAR_CLIENT_ERROR("Unknown mouse button action: {0}", action);
			return; // 不处理未知动作
		};
	}
	
	Bear::WindowsWindow::WindowsWindow(const WindowProps& props)
	{
		Init(props);
	}
	WindowsWindow::~WindowsWindow()
	{
		Shutdown();
	}
	void WindowsWindow::OnUpdate()
	{
		glfwPollEvents(); // 处理所有待处理的事件
		glfwSwapBuffers(m_Window); // 交换前后缓冲区，显示渲染结果
	}
	void WindowsWindow::SetVSync(bool enabled)
	{
		if (enabled) {
			glfwSwapInterval(1); // 启用垂直同步，交换缓冲区时等待显示器刷新率
		}
		else {
			glfwSwapInterval(0); // 禁用垂直同步，立即交换缓冲区
		}
		m_Data.VSync = enabled;
	}
	bool WindowsWindow::IsVSync() const
	{
		return m_Data.VSync;
	}
	void WindowsWindow::Init(const WindowProps& props)
	{
		m_Data.Title = props.Title;
		m_Data.Width = props.Width;
		m_Data.Height = props.Height;

		if (!s_GLFWInitialized)
		{
			int success = glfwInit();
			BEAR_CORE_ASSERT(success, "Could not initialize GLFW!");
			s_GLFWInitialized = true;
		}

		if (props.UseVulkan) {
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // 禁用OpenGL，使用Vulkan
			glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE); // 允许窗口调整大小
			m_Window = glfwCreateWindow((int)m_Data.Width, (int)m_Data.Height, m_Data.Title.c_str(), nullptr, nullptr);
		}
		else {
			glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API); // 使用OpenGL
			m_Window = glfwCreateWindow((int)m_Data.Width, (int)m_Data.Height, m_Data.Title.c_str(), nullptr, nullptr);
			glfwMakeContextCurrent(m_Window);
			int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
			BEAR_CORE_ASSERT(status, "Failed to initialize Glad!");
		}

		// 设置GLFW窗口的用户数据为WindowData结构体的指针，这里主要是为了glfw在检测到事件调用回调函数时，通过glfwGetWindowUserPointer获取到这个指针（&m_Data），然后根据事件进行包装调用具体的事件处理函数
		glfwSetWindowUserPointer(m_Window, &m_Data);
		SetVSync(true);

		// callbacks
		glfwSetWindowSizeCallback(m_Window, WindowSizeCallback);
		glfwSetWindowCloseCallback(m_Window, WindowCloseCallback);

		glfwSetKeyCallback(m_Window, KeyCallback);

		glfwSetCursorPosCallback(m_Window, MouseMoveCallback);
		glfwSetScrollCallback(m_Window, MouseScrollCallback);
		glfwSetMouseButtonCallback(m_Window, MouseButtonCallback);
	}
	void WindowsWindow::Shutdown()
	{
		glfwDestroyWindow(m_Window);
		s_GLFWInitialized = false;
	}
}
