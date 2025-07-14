#pragma once

#include "bearpch.h"
#include "Bear/Core.h"

namespace Bear {

	struct WindowProps {
		std::string Title;
		unsigned int Width;
		unsigned int Height;
		WindowProps(const std::string& title = "Bear Engine", unsigned int width = 1280, unsigned int height = 720)
			: Title(title), Width(width), Height(height) {}
	};

	class BEAR_API Window {
	public:
			Window(const WindowProps& props = WindowProps());
			virtual ~Window() {}
			virtual void OnUpdate() = 0;
            // 包含virtual的是虚函数，=0表示纯虚函数，必须在派生类中实现，且包含纯虚函数的类是抽象类，不能实例化
			virtual unsigned int GetWidth() const = 0;
			virtual unsigned int GetHeight() const = 0;
			virtual void SetEventCallback(const std::function<void(Event&)>& callback) = 0;
			virtual void SetVSync(bool enabled) = 0;
			virtual bool IsVSync() const = 0;

			static Window* Create(const WindowProps& props = WindowProps());
	}
}