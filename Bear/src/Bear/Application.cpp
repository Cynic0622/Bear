#include "bearpch.h"
#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Application.h"
#include "Log.h"

#include <glad/glad.h>

namespace Bear {

	Application* Application::s_Instance = nullptr;
	Application::Application()
	{
		m_Window = std::unique_ptr<Window>(Window::Create(WindowProps("Bear", 1280, 720)));
		m_Window->SetEventCallback([this](Event& e) {
			this->OnEvent(e);
			});
		s_Instance = this;
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
			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			// Calculate delta time
			static int lastTime = 0;
			int currentTime = static_cast<int>(glfwGetTime() * 1000); // Convert to milliseconds
			deltaTime = currentTime - lastTime;
			
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
		layer->OnAttach();
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


}