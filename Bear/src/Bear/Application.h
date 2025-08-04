#pragma once
#include "Core.h"
#include "Window.h"
#include "Events/ApplicationEvent.h"
#include "LayerStack.h"
#include "Vulkan/Core/Instance.h"
#include  "Renderer.h"

namespace Bear {

	class VulkanRenderer;
	class BEAR_API Application
	{
	public:
		Application();
		virtual ~Application();

		void OnEvent(Event& e);

		void Run();

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);

		inline Window& GetWindow() { return *m_Window; } // 获取窗口的引用，用于访问窗口相关功能
		inline static Application& Get() { return *s_Instance; } // 获取应用程序实例的静态方法
		inline Renderer* GetRenderer() { return m_Renderer.get(); } // 获取渲染设备的引用
		
	private:
		std::unique_ptr<Window> m_Window; // 使用智能指针管理窗口对象的生命周期，有唯一窗口指针
		bool m_Running = true;

		static Application* s_Instance; // 静态实例指针，用于访问应用程序实例
		//std::unique_ptr<VulkanRenderer> m_VulkanRenderer; // 使用智能指针管理 Vulkan 渲染器的生命周期
		std::unique_ptr<Renderer> m_Renderer;
		LayerStack m_LayerStack; // 层栈，用于管理应用程序的层
	private:
		// 事件处理函数
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

	};

	// 在 SandboxApp.cpp 中实现
	Application* CreateApplication();
}