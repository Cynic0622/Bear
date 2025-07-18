#pragma once
#include "Core.h"
#include "Window.h"
#include "Events/ApplicationEvent.h"
#include "LayerStack.h"

namespace Bear {

	class BEAR_API Application
	{
	public:
		Application();
		virtual ~Application();

		void OnEvent(Event& e);

		void Run();

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);

	private:
		std::unique_ptr<Window> m_Window; // 使用智能指针管理窗口对象的生命周期，有唯一窗口指针
		bool m_Running = true;

		LayerStack m_LayerStack; // 层栈，用于管理应用程序的层

	private:
		// 事件处理函数
		bool OnWindowClose(WindowCloseEvent& e);
		//bool OnWindowResize(WindowResizeEvent& e);
	};

	// 在 SandboxApp.cpp 中实现
	Application* CreateApplication();
}