#include "bearpch.h"
#include "GuiLayer.h"
#include "Bear/Application.h"

namespace Bear {
    
    GuiLayer::GuiLayer() : Layer("GuiLayer") {}

    
    void GuiLayer::OnAttach() {

    }
    
    void GuiLayer::OnDetach() {
    }

    bool GuiLayer::OnMouseButtonPress(MouseButtonPressedEvent& event)
    {
        ImGuiIO& io = ImGui::GetIO();
		return io.WantCaptureMouse; // if ImGui wants to capture the mouse input, return true
    }

    bool GuiLayer::OnWindowResize(WindowResizeEvent& event)
    {
        return false;
    }

    void GuiLayer::OnUpdate(float deltaTime) {
        
        ImGui::Begin("Renderer Info");
        ImGui::Text("Graphics api: Vulkan");
        ImGui::Text("Frame time: %.3f ms", deltaTime * 1000.0f);
        ImGui::Text("FPS: %.1f", 1.0f / deltaTime);
        ImGui::End();
        
    }
    
    void GuiLayer::OnEvent(Event& event) {

		EventDispatcher dispatcher(event);
        dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& e)
            {
				return this->OnMouseButtonPress(e);
            });
    }

    void GuiLayer::OnRender() const
    {
    }

}