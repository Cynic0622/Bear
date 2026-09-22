#include "bearpch.h"
#include "Events/ApplicationEvent.h"
#include "Application.h"

#include "Renderer/Renderer.h"
#include "Renderer/RHI/RHITypes.h"

#include <chrono>
#include <thread>

namespace Bear
{
	Application* Application::s_Instance = nullptr;

	Application::Application()
	{
		m_Window = std::unique_ptr<Window>(Window::Create(WindowProps("Bear", 1280, 720)));
		m_Window->SetEventCallback([this](Event& e)
		{
			this->OnEvent(e);
		});
		s_Instance = this;
		m_Renderer = std::make_unique<Renderer>(static_cast<GLFWwindow*>(m_Window->GetNativeWindow()),
		                                        GraphicsAPI::Vulkan);
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
		m_Renderer->OnEvent(e);
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
		float lastTime = static_cast<float>(glfwGetTime());
		while (m_Running)
		{
			// Minimized: the framebuffer is 0x0, so there is no valid swapchain to draw or present
			// into. Keep pumping events (that is how the restore is noticed) and wait for one.
			if (m_Window->IsMinimized())
			{
				m_Window->OnUpdate();
				// Do not carry the minimized period into the next delta time.
				lastTime = static_cast<float>(glfwGetTime());
				std::this_thread::sleep_for(std::chrono::milliseconds(16));
				continue;
			}

			// Calculate delta time
			float currentTime = static_cast<float>(glfwGetTime());
			deltaTime = currentTime - lastTime;
			m_Renderer->BeginFrame();
			for (Layer* layer : m_LayerStack)
			{
				layer->OnUpdate(deltaTime);
			}
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
		// The window layer only reports real sizes (>= 1x1); guard anyway instead of asserting, so a
		// minimized window can never reach the swapchain as a 0x0 extent.
		if (e.GetWidth() <= 0 || e.GetHeight() <= 0)
			return false;

		BEAR_CORE_TRACE("WindowResizeEvent: {0}, {1}", e.GetWidth(), e.GetHeight());
		m_Renderer->OnWindowResize();
		return false;
	}
}
