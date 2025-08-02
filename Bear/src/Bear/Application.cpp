#include "bearpch.h"
#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Application.h"
#include "Log.h"

#include <glad/glad.h>
#include "Pipeline/VulkanShader.h"
#include "Pipeline/PipelineConfig.h"
#include "VulkanRenderer.h"

namespace Bear {

	Application* Application::s_Instance = nullptr;
	Application::Application()
	{
		m_Window = std::unique_ptr<Window>(Window::Create(WindowProps("Bear", 1280, 720)));
		m_Window->SetEventCallback([this](Event& e) {
			this->OnEvent(e);
			});
		s_Instance = this;
		m_VulkanRenderer = std::make_unique<VulkanRenderer>(static_cast<GLFWwindow*>(m_Window->GetNativeWindow()));
	}
	Application::~Application()
	{
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		bool isHandled =  dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& e) { return this->OnWindowClose(e); });
		//BEAR_CORE_TRACE("Event: {0}", e);
		if (isHandled)
			return;
		isHandled = dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e) { return this->OnWindowResize(e); });
		if (isHandled)
			return;
		for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
		{
			(*it)->OnEvent(e);
			if (e.IsHandled())
				return;
		}
	}

	void Application::Run()
	{
		int deltaTime = 0;
		while (m_Running)
		{
			// Calculate delta time
			static int lastTime = 0;
			int currentTime = static_cast<int>(glfwGetTime() * 1000); // Convert to milliseconds
			deltaTime = currentTime - lastTime;
			
			m_VulkanRenderer->DrawFrame();

			for (Layer* layer : m_LayerStack)
			{
				layer->OnUpdate(deltaTime);
			}
			lastTime = currentTime;

			m_Window->OnUpdate();
		}
	}

	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
		//layer->OnAttach();
	}

	void Application::PushOverlay(Layer* overlay)
	{
		m_LayerStack.PushOverlay(overlay);
		//overlay->OnAttach();
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		BEAR_CORE_TRACE("WindowCloseEvent: {0}", e);
		m_Running = false;
		return false;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		BEAR_CORE_ASSERT(e.GetWidth() > 0 && e.GetHeight() > 0, "Window resize event with invalid dimensions!");
		BEAR_CORE_TRACE("WindowResizeEvent: {0}, {1}", e.GetWidth(), e.GetHeight());
		m_VulkanRenderer->OnWindowResized();
		return true;
	}


}