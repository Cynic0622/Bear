#include "bearpch.h"
#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Application.h"
#include "Log.h"
namespace Bear {

	Application::Application()
	{
	}
	Application::~Application()
	{
	}

	void Application::Run()
	{
		Bear::WindowResizeEvent e(1280, 720);
		BEAR_CORE_INFO(e);
		BEAR_CORE_INFO(e.ToString());
		KeyPressedEvent e2(32, 0);
		BEAR_CORE_INFO(e2);
		BEAR_CORE_INFO(e2.ToString());
		while (true)
		{
			//printf("helloxx!");
		}
	}

}