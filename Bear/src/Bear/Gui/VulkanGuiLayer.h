#pragma once

#include "Bear/Layer.h"
#include "Bear/Events/Event.h"
#include "Bear/Events/ApplicationEvent.h"
#include "Bear/Events/KeyEvent.h"
#include "Bear/Events/MouseEvent.h"

// Vulkan headers
#include <vulkan/vulkan.h>

// Forward declarations
struct GLFWwindow;

namespace Bear {
    
    // Forward declarations
    class VulkanDevice;
    class VulkanSurface;
    class VulkanSwapchain;
    class VulkanRenderPass;
    
    class VulkanGuiLayer : public Layer {
    public:
        VulkanGuiLayer();
        virtual ~VulkanGuiLayer();

        virtual void OnAttach() override;
        virtual void OnDetach() override;
        virtual void OnUpdate(float deltaTime) override;
        virtual void OnEvent(Event& event) override;

    private:
        void InitImGui();
        void ShutdownImGui();
        void UploadFonts();
        
        // Vulkan错误检查回调
        static void CheckVkResult(VkResult err);

    private:
        // Vulkan对象引用（不拥有所有权）
        VulkanDevice* m_VulkanDevice = nullptr;
        VulkanSurface* m_VulkanSurface = nullptr;
        VulkanSwapchain* m_VulkanSwapchain = nullptr;
        VulkanRenderPass* m_VulkanRenderPass = nullptr;
		VulkanInstance* m_VulkanInstance = nullptr;
        
        // ImGui专用的Vulkan资源
        VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
        VkCommandPool m_CommandPool = VK_NULL_HANDLE;
        VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
        
        // 同步对象
        VkSemaphore m_ImageAvailableSemaphore = VK_NULL_HANDLE;
        VkSemaphore m_RenderFinishedSemaphore = VK_NULL_HANDLE;
        VkFence m_InFlightFence = VK_NULL_HANDLE;
        
        uint32_t m_CurrentFrame = 0;
        bool m_SwapChainRebuild = false;
    };
}