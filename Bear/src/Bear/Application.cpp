#include "bearpch.h"
#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Application.h"
#include "Log.h"

namespace Bear {

	Application::Application()
	{
		m_Window = std::unique_ptr<Window>(Window::Create(WindowProps("Bear", 1280, 720)));
		m_Window->SetEventCallback([this](Event& e) {
			this->OnEvent(e);
			});
	}
	Application::~Application()
	{
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& e) { return this->OnWindowClose(e); });
		BEAR_CORE_TRACE("Event: {0}", e);
	}

	void Application::Run()
	{
		while (m_Running)
		{
			m_Window->OnUpdate();
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		BEAR_CORE_TRACE("WindowCloseEvent: {0}", e);
		m_Running = false;
		return false;
	}


}