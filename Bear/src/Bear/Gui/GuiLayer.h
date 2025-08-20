#pragma once

#include "Bear/Layer.h"
#include "Bear/Events/Event.h"
#include "Bear/Events/ApplicationEvent.h"
#include "Bear/Events/KeyEvent.h"
#include "Bear/Events/MouseEvent.h"

#include <vulkan/vulkan.h>

namespace Bear
{
	class RHICommandList;
}

// Forward declarations
struct GLFWwindow;

namespace Bear {
	class RHIDevice;

	// Forward declarations
    class Device;
    
    class BEAR_API GuiLayer : public Layer {
    public:
        GuiLayer();
    	~GuiLayer() override = default;

        GuiLayer(const GuiLayer&) = delete;
		GuiLayer& operator=(const GuiLayer&) = delete;

    	void OnAttach() override;
    	void OnDetach() override;
    	void OnUpdate(float deltaTime) override;
    	void OnEvent(Event& event) override;
        void OnRender() const override;

    private:

		bool OnMouseButtonPress(MouseButtonPressedEvent& event);
		bool OnWindowResize(WindowResizeEvent& event);


    private:
		RHIDevice* m_Device = nullptr;
        
        // ImGui专用的Vulkan资源
        VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    };
}