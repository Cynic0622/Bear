#include "bearpch.h"
#include "Events/ApplicationEvent.h"
#include "Application.h"

#include "Renderer/Renderer.h"
#include "Renderer/RHI/RHITypes.h"
namespace Bear {

	Application* Application::s_Instance = nullptr;
	Application::Application()
	{
		m_Window = std::unique_ptr<Window>(Window::Create(WindowProps("Bear", 1280, 720)));
		m_Window->SetEventCallback([this](Event& e) {
			this->OnEvent(e);
			});
		s_Instance = this;
		m_Renderer = std::make_unique<Renderer>(static_cast<GLFWwindow*>(m_Window->GetNativeWindow()), GraphicsAPI::Vulkan);
		Input::Init(static_cast<GLFWwindow*>(m_Window->GetNativeWindow()));
	}
	Application::~Application()
	{
		if (m_Renderer)
		{
			m_Renderer->GetDevice()->WaitIdle();
		}
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& e) { return this->OnWindowClose(e); });
		
		dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e) { return this->OnWindowResize(e); });

		if (e.IsHandled()) return;
		
		for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
		{
			(*it)->OnEvent(e);
			if (e.IsHandled())
				return;
		}
	}

	void Application::Run()
	{
		float deltaTime = 0;
		while (m_Running)
		{
			// Calculate delta time
			static float lastTime = 0;
			float currentTime = static_cast<float>(glfwGetTime());
			deltaTime = currentTime - lastTime;
			
			for (Layer* layer : m_LayerStack)
			{
				layer->OnUpdate(deltaTime);
			}

			m_Renderer->BeginFrame();
			for (Layer* layer : m_LayerStack)
			{
				layer->OnRender();
			}
			m_Renderer->EndFrame();

			m_Window->OnUpdate();

			lastTime = currentTime;
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
		m_Renderer->OnWindowResized();
		return false;
	}
}