#include "bearpch.h"
#include "VulkanGuiLayer.h"
#include "Bear/Application.h"

// Vulkan backend includes
#include "Renderer/Backend/Vulkan/Core/Device.h"
#include "Renderer/Backend/Vulkan/Pipeline/RenderPass.h"
#include "Renderer/Backend/Vulkan/Command/CommandBuffer.h"
#include "Renderer/RHI/RHICommandList.h"

// ImGui includes
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include <imgui_impl_vulkan.h>


namespace Bear {
    
    VulkanGuiLayer::VulkanGuiLayer() : Layer("VulkanGuiLayer") {}
    
    VulkanGuiLayer::~VulkanGuiLayer() {}
    
    void VulkanGuiLayer::OnAttach() {

        InitImGui();
        BEAR_CORE_INFO("VulkanGuiLayer: Initialized successfully");
    }
    
    void VulkanGuiLayer::OnDetach() {
        BEAR_CORE_INFO("VulkanGuiLayer: Shutting down...");

        ShutdownImGui();
        
        BEAR_CORE_INFO("VulkanGuiLayer: Shutdown complete");
    }
    
    void VulkanGuiLayer::InitImGui() {
        // 1. 创建ImGui上下文
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        
        // 2. 配置ImGui IO
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiBackendFlags_HasMouseCursors; // Enable Mouse Cursors
        io.ConfigFlags |= ImGuiBackendFlags_HasSetMousePos; // Enable SetMousePos backend function
        
        // 3. 初始化GLFW后端
        Application& app = Application::Get();
        GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());
        ImGui_ImplGlfw_InitForVulkan(window, true);
        auto* renderer = app.GetRenderer();
        m_Device = renderer->GetDevice();
        auto* vkDevice = dynamic_cast<Device*>(m_Device);
        auto* vkRenderPass = dynamic_cast<RenderPass*>(renderer->GetRenderPass());
        
		// 4. 创建描述符池（ImGui需要）
        VkDescriptorPoolSize pool_sizes[] = {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };
     
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000;
        pool_info.poolSizeCount = std::size(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        
        VkResult result = vkCreateDescriptorPool(vkDevice->GetDevice(), &pool_info, nullptr, &m_DescriptorPool);
        BEAR_CORE_ASSERT(result == VK_SUCCESS, "Failed to create descriptor pool for ImGui!");
        
        // 8. 初始化ImGui Vulkan后端
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = vkDevice->GetInstance().GetHandle();
        init_info.PhysicalDevice = vkDevice->GetPhysicalDevice();
        init_info.Device = vkDevice->GetDevice();
        init_info.QueueFamily = vkDevice->GetQueueFamilyIndices().graphicsFamily.value();
        init_info.Queue = vkDevice->GetGraphicsQueue();
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.DescriptorPool = m_DescriptorPool;
		init_info.RenderPass = vkRenderPass->GetHandle();
        init_info.Subpass = 0;
        init_info.MinImageCount = renderer->GetSwapchainImageCount();
        init_info.ImageCount = renderer->GetSwapchainImageCount();
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator = nullptr;
        
        ImGui_ImplVulkan_Init(&init_info);
    }
    
    void VulkanGuiLayer::ShutdownImGui() {
        if (!m_Device) return;

		auto* vkDevice = dynamic_cast<Device*>(m_Device);
        vkDevice->WaitIdle();

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        if (m_DescriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(vkDevice->GetDevice(), m_DescriptorPool, nullptr);
            m_DescriptorPool = VK_NULL_HANDLE;
        }
    }
    
    void VulkanGuiLayer::OnUpdate(float deltaTime) {
        
        // 开始ImGui新帧
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // 渲染GUI内容
        ImGui::ShowDemoWindow();
        
        // 自定义GUI示例
        ImGui::Begin("Vulkan Info");
        ImGui::Text("Renderer: Vulkan");
        ImGui::Text("Frame time: %.3f ms", deltaTime * 1000.0f);
        ImGui::Text("FPS: %.1f", 1.0f / deltaTime);
        ImGui::End();
        
        // 渲染ImGui
        ImGui::Render();
    }
    
    void VulkanGuiLayer::OnEvent(Event& event) {
        /*ImGuiIO& io = ImGui::GetIO();
        event.Handled |= event.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
        event.Handled |= event.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;*/
    }

    void VulkanGuiLayer::OnRender(RHICommandList& cmd) const
    {
        ImDrawData* draw_data = ImGui::GetDrawData();
        if (!draw_data || draw_data->CmdListsCount == 0) return;

        auto& vkCmd = static_cast<CommandBuffer&>(cmd);
        ImGui_ImplVulkan_RenderDrawData(draw_data, vkCmd.GetHandle());
    }

}