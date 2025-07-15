#pragma once
#include "Core.h"
#include "Window.h"

namespace Bear {

	class BEAR_API Application
	{
	public:
		Application();
		virtual ~Application();
		void Run();

	private:
		std::unique_ptr<Window> m_Window; // 使用智能指针管理窗口对象的生命周期，有唯一窗口指针
		bool m_Running = true;
	};

	// 在 SandboxApp.cpp 中实现
	Application* CreateApplication();
}