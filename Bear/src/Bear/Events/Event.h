#pragma once

#include "Bear/bearpch.h"
#include "Bear/Core.h"
#include <spdlog/fmt/ostr.h> 

namespace Bear {

	// Events in Hazel are currently blocking, meaning when an event occurs it
	// immediately gets dispatched and must be dealt with right then an there.
	// For the future, a better strategy might be to buffer events in an event
	// bus and process them during the "event" part of the update stage.

	enum class EventType
	{
		None = 0,
		WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
		AppTick, AppUpdate, AppRender,
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
	};

	// 这里使用位运算，是因为考虑到事件可能属于多个类别，比如 EventCategoryInput | EventCategoryKeyboard; ---> 0010 | 0100 = 0110
	enum EventCategory
	{
		None = 0,
		EventCategoryApplication = BIT(0),
		EventCategoryInput = BIT(1),
		EventCategoryKeyboard = BIT(2),
		EventCategoryMouse = BIT(3),
		EventCategoryMouseButton = BIT(4)
	};

#define EVENT_CLASS_TYPE(type) static EventType GetStaticType() { return EventType::type; }\
								virtual EventType GetEventType() const override { return GetStaticType(); }\
								virtual const char* GetName() const override { return #type; }

#define EVENT_CLASS_CATEGORY(category) virtual int GetCategoryFlags() const override { return category; }

	class BEAR_API Event
	{
		// friend class EventDispatcher;
		friend class EventDispatcher;
	public:
		virtual ~Event() = default;
		virtual EventType GetEventType() const = 0;
		virtual const char* GetName() const = 0;
		virtual int GetCategoryFlags() const = 0;
		virtual std::string ToString() const { return GetName(); }

		bool IsInCategory(EventCategory category)
		{
			return GetCategoryFlags() & category;
		}
		
		bool IsHandled() const { return Handled; }
	protected:
		// 事件处理标志，表示事件是否被处理
		bool Handled = false;
	};

	// 根据事件类型自动调用对应的处理函数
	class BEAR_API EventDispatcher
	{
	public:
		EventDispatcher(Event& event)
			: m_Event(event){}

		// T 是事件类型，F 是处理函数类型，比如lambda函数或函数指针; func 是处理函数的实例
		template<typename T, typename F>
		bool Dispatch(const F& func)
		{
			if (m_Event.GetEventType() == T::GetStaticType())
			{
				m_Event.Handled |= func(static_cast<T&>(m_Event));
				//return true;
			}
			return false;
		}
	private:
		Event& m_Event;
	};

	/*inline std::ostream& operator<<(std::ostream& os, const Event& e)
	{
		return os << e.ToString();
	}*/
	template <typename T>
	struct fmt::formatter<T, std::enable_if_t<std::is_base_of_v<Bear::Event, T>, char>> {
		template <typename ParseContext>
		constexpr auto parse(ParseContext& ctx) -> decltype(ctx.begin()) {
			return ctx.begin();
		}

		template <typename FormatContext>
		auto format(const T& event, FormatContext& ctx) const -> decltype(ctx.out()) {
			return fmt::format_to(ctx.out(), "{}", event.ToString());
		}
	};
}