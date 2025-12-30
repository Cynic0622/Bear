#pragma once

#include "bearpch.h"
#include "Bear/Core.h"
#include "Bear/Events/Event.h"

namespace Bear {

	struct WindowProps 
	{
		std::string Title;
		unsigned int Width;
		unsigned int Height;
		bool UseVulkan = true;
		WindowProps(const std::string& title = "Bear Engine", unsigned int width = 1280, unsigned int height = 720)
			: Title(title), Width(width), Height(height) {}
	};

	//Window(const WindowProps& props = WindowProps());
		// 类型别名 using 新类型名 = 原类型定义;
		/*
		@param EventCallbackFn: 定义一个新的类型别名，表示一个接受Event引用参数并返回void的可调用对象类型。
		@param Event: 事件类，表示各种事件的基类。
		@param std::function: C++标准库中的函数对象包装器，可以存储任何可调用对象（函数、lambda表达式、函数指针等）。
		*/
	using EventCallbackFn = std::function<void(Event&)>;

	struct WindowData
	{
		std::string Title;
		unsigned int Width, Height;
		bool VSync;

		EventCallbackFn EventCallback;
	};

	class BEAR_API Window {
	public:

		virtual ~Window() {}
		virtual void OnUpdate() = 0;
		// 包含virtual的是虚函数，=0表示纯虚函数，必须在派生类中实现，且包含纯虚函数的类是抽象类，不能实例化
		virtual unsigned int GetWidth() const = 0;
		virtual unsigned int GetHeight() const = 0;
		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
		virtual void SetVSync(bool enabled) = 0;
		virtual bool IsVSync() const = 0;
		virtual void* GetNativeWindow() const = 0; // 返回原生窗口指针，通常是平台相关的窗口句柄或指针

		static Window* Create(const WindowProps& props = WindowProps());
	};
}