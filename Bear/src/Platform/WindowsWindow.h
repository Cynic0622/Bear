#pragma once

#include "Bear/Window.h"
#include <GLFW/glfw3.h>

namespace Bear {

	class WindowsWindow : public Window
	{
	public:
		WindowsWindow(const WindowProps& props);
		virtual ~WindowsWindow();

		void OnUpdate() override;

		inline unsigned int GetWidth() const override { return m_Data.Width; }
		inline unsigned int GetHeight() const override { return m_Data.Height; }

		// Window attributes
		inline void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
		void SetVSync(bool enabled) override;
		// 这里的const 表示这个函数只可以读取而不能修改成员变量，同理也不能调用非const的成员函数
		// 这意味着这个函数不会改变对象的状态
		inline bool IsVSync() const override;

	private:
		// 设为private是为了防止其他类直接访问这个函数，通过public基类函数调用
		virtual void Init(const WindowProps& props);
		virtual void Shutdown();
	private:
		// m 表示member，表示这是一个成员变量
		GLFWwindow* m_Window;

		struct WindowData
		{
			std::string Title;
			unsigned int Width, Height;
			bool VSync;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;
	};

}