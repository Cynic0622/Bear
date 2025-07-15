#include "bearpch.h"
#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Application.h"
#include "Log.h"
namespace Bear {

	Application::Application()
	{
		m_Window = std::unique_ptr<Window>(Window::Create(WindowProps("Bear", 1280, 720)));
	}
	Application::~Application()
	{
	}

	void Application::Run()
	{
		while (m_Running)
		{
			m_Window->OnUpdate();
		}
	}

}