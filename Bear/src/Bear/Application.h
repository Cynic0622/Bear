#pragma once
#include "Core.h"

namespace Bear {

	class BEAR_API Application
	{
	public:
		Application();
		virtual ~Application();
		void Run();
	};

	// 在 SandboxApp.cpp 中实现
	Application* CreateApplication();
}