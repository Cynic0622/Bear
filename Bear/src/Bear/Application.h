#pragma once
#include "Core.h"
#include "Window.h"
#include "Events/ApplicationEvent.h"
#include "LayerStack.h"
#include "Vulkan/Core/Instance.h"
#include "Vulkan/Presentation/Surface.h"
#include "Vulkan/Presentation/Swapchain.h"
#include "Vulkan/Core/Device.h"
#include "Vulkan/Pipeline/RenderPass.h"
#include "Vulkan/Pipeline/Pipeline.h"

namespace Bear {

	class VulkanRenderer;
	class Renderer;
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
		//inline Instance* GetVulkanInstance() { return m_VulkanInstance.get(); } // 获取 Vulkan 实例的引用
		//inline Surface* GetVulkanSurface() { return m_VulkanSurface.get(); } // 获取 Vulkan 表面的引用
		//inline Device* GetVulkanDevice() { return m_VulkanDevice.get(); } // 获取 Vulkan 设备的引用
		//inline Swapchain* GetVulkanSwapchain() { return m_VulkanSwapchain.get(); } // 获取 Vulkan 交换链的引用
		//inline RenderPass* GetVulkanRenderPass() { return m_VulkanRenderPass.get(); } // 获取 Vulkan 渲染通道的引用
		//inline VulkanRenderer* GetVulkanRenderer() { return m_VulkanRenderer.get(); } // 获取 Vulkan 渲染器的引用
	private:
		std::unique_ptr<Window> m_Window; // 使用智能指针管理窗口对象的生命周期，有唯一窗口指针
		bool m_Running = true;

		LayerStack m_LayerStack; // 层栈，用于管理应用程序的层

		static Application* s_Instance; // 静态实例指针，用于访问应用程序实例
		//std::unique_ptr<VulkanRenderer> m_VulkanRenderer; // 使用智能指针管理 Vulkan 渲染器的生命周期
		std::unique_ptr<Renderer> m_Renderer;
	private:
		// 事件处理函数
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);
		
		//std::unique_ptr<Instance> m_VulkanInstance; // 使用智能指针管理 Vulkan 实例的生命周期
		//std::unique_ptr<Surface> m_VulkanSurface; // 使用智能指针管理 Vulkan 表面的生命周期
		//std::unique_ptr<Device> m_VulkanDevice; // 使用智能指针管理 Vulkan 设备的生命周期
		//std::unique_ptr<Swapchain> m_VulkanSwapchain; // 使用智能指针管理 Vulkan 交换链的生命周期
		//std::unique_ptr<RenderPass> m_VulkanRenderPass; // 使用智能指针管理 Vulkan 渲染通道的生命周期
		//std::unique_ptr<Pipeline> m_VulkanPipeline; // 使用智能指针管理 Vulkan 管线的生命周期
	};

	// 在 SandboxApp.cpp 中实现
	Application* CreateApplication();
}