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
    
    class BEAR_API VulkanGuiLayer : public Layer {
    public:
        VulkanGuiLayer();
    	~VulkanGuiLayer() override = default;

        VulkanGuiLayer(const VulkanGuiLayer&) = delete;
		VulkanGuiLayer& operator=(const VulkanGuiLayer&) = delete;

    	void OnAttach() override;
    	void OnDetach() override;
    	void OnUpdate(float deltaTime) override;
    	void OnEvent(Event& event) override;
        void OnRender() const override;

    private:
        void InitImGui();
        void ShutdownImGui();


    private:
		RHIDevice* m_Device = nullptr;
        
        // ImGui专用的Vulkan资源
        VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    };
}