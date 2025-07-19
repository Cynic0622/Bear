#pragma once

#include "Layer.h"

namespace Bear {

	class BEAR_API GuiLayer : public Layer
	{
	public:
		GuiLayer();
		~GuiLayer();

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(float deltaTime) override;
		void OnEvent(Event& event) override;
	};
}