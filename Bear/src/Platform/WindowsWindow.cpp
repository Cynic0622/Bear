#include "bearpch.h"
#include "WindowsWindow.h"
#include "Bear/Core.h"
#include "Assert.h"

namespace Bear {
	static bool s_GLFWInitialized = false;

	Window* Window::Create(const WindowProps& props)
	{
		return new WindowsWindow(props);
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

		BEAR_CLIENT_INFO("Creating window {0} ({1}, {2})", m_Data.Title, m_Data.Width, m_Data.Height);

		m_Window = glfwCreateWindow((int)m_Data.Width, (int)m_Data.Height, m_Data.Title.c_str(), nullptr, nullptr);
		glfwMakeContextCurrent(m_Window);
		// 设置GLFW窗口的用户数据为WindowData结构体的指针，这里主要是为了glfw在检测到事件调用回调函数时，通过glfwGetWindowUserPointer获取到这个指针（&m_Data），然后根据事件进行包装调用具体的事件处理函数
		glfwSetWindowUserPointer(m_Window, &m_Data);
		SetVSync(true);
	}
	void WindowsWindow::Shutdown()
	{
		glfwDestroyWindow(m_Window);
		s_GLFWInitialized = false;
	}
}
