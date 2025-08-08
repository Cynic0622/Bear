#pragma once

#include "Core.h"
#include "Events/Event.h"

namespace Bear {
	class RHICommandList;
	class Renderer;
	class BEAR_API Layer
	{
	public:
		Layer(std::string name = "Layer") : m_DebugName(std::move(name)) {}
		virtual ~Layer() = default;

		Layer(const Layer&) = delete;
		Layer& operator=(const Layer&) = delete;

		virtual void OnAttach() = 0;
		virtual void OnDetach() = 0;
		virtual void OnUpdate(float deltaTime) = 0;
		virtual void OnEvent(Event& event) {} // temp, must be overridden in derived classes later.
		virtual void OnRender() const {}
		const std::string& GetName() const { return m_DebugName; }
	protected:
		std::string m_DebugName;
	};
}