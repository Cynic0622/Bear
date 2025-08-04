#pragma once

#include "Bear/Layer.h"
#include "Bear/Events/Event.h"
#include "Bear/Events/ApplicationEvent.h"
#include "Bear/Events/KeyEvent.h"
#include "Bear/Events/MouseEvent.h"

// Vulkan headers
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
    
    class BEAR_API VulkanGuiLayer : public Layer {
    public:
        VulkanGuiLayer();
        virtual ~VulkanGuiLayer() override;

        virtual void OnAttach() override;
        virtual void OnDetach() override;
        virtual void OnUpdate(float deltaTime) override;
        virtual void OnEvent(Event& event) override;
        void OnRender(RHICommandList& cmd) const;

    private:
        void InitImGui();
        void ShutdownImGui();


    private:
		RHIDevice* m_Device = nullptr;
        
        // ImGui专用的Vulkan资源
        VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    };
}