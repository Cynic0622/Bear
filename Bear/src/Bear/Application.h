#pragma once
#include "Core.h"
#include "Window.h"
#include "Events/ApplicationEvent.h"
#include "LayerStack.h"
#include "Vulkan/Core/VulkanInstance.h"
#include "Vulkan/Presentation/VulkanSurface.h"
#include "Vulkan/Presentation/VulkanSwapchain.h"
#include "Vulkan/Core/VulkanDevice.h"
#include "Vulkan/Pipeline/VulkanRenderPass.h"
#include "Vulkan/Pipeline/VulkanPipeline.h"

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
		inline VulkanInstance* GetVulkanInstance() { return m_VulkanInstance.get(); } // 获取 Vulkan 实例的引用
		inline VulkanSurface* GetVulkanSurface() { return m_VulkanSurface.get(); } // 获取 Vulkan 表面的引用
		inline VulkanDevice* GetVulkanDevice() { return m_VulkanDevice.get(); } // 获取 Vulkan 设备的引用
		inline VulkanSwapchain* GetVulkanSwapchain() { return m_VulkanSwapchain.get(); } // 获取 Vulkan 交换链的引用
		inline VulkanRenderPass* GetVulkanRenderPass() { return m_VulkanRenderPass.get(); } // 获取 Vulkan 渲染通道的引用
		inline VulkanRenderer* GetVulkanRenderer() { return m_VulkanRenderer.get(); } // 获取 Vulkan 渲染器的引用
	private:
		std::unique_ptr<Window> m_Window; // 使用智能指针管理窗口对象的生命周期，有唯一窗口指针
		bool m_Running = true;

		LayerStack m_LayerStack; // 层栈，用于管理应用程序的层

		static Application* s_Instance; // 静态实例指针，用于访问应用程序实例
		std::unique_ptr<VulkanRenderer> m_VulkanRenderer; // 使用智能指针管理 Vulkan 渲染器的生命周期
	private:
		// 事件处理函数
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);
		
		std::unique_ptr<VulkanInstance> m_VulkanInstance; // 使用智能指针管理 Vulkan 实例的生命周期
		std::unique_ptr<VulkanSurface> m_VulkanSurface; // 使用智能指针管理 Vulkan 表面的生命周期
		std::unique_ptr<VulkanDevice> m_VulkanDevice; // 使用智能指针管理 Vulkan 设备的生命周期
		std::unique_ptr<VulkanSwapchain> m_VulkanSwapchain; // 使用智能指针管理 Vulkan 交换链的生命周期
		std::unique_ptr<VulkanRenderPass> m_VulkanRenderPass; // 使用智能指针管理 Vulkan 渲染通道的生命周期
		std::unique_ptr<VulkanPipeline> m_VulkanPipeline; // 使用智能指针管理 Vulkan 管线的生命周期
	};

	// 在 SandboxApp.cpp 中实现
	Application* CreateApplication();
}