#include "bearpch.h"
#include "VulkanGuiLayer.h"
#include "Bear/Application.h"

// Vulkan backend includes
#include "Renderer/Backend/Vulkan/Core/VulkanDevice.h"
#include "Renderer/Backend/Vulkan/Presentation/VulkanSurface.h"
#include "Renderer/Backend/Vulkan/Presentation/VulkanSwapchain.h"
#include "Renderer/Backend/Vulkan/Pipeline/VulkanRenderPass.h"

// ImGui includes
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <GLFW/glfw3.h>

namespace Bear {
    
    VulkanGuiLayer::VulkanGuiLayer() : Layer("VulkanGuiLayer") {}
    
    VulkanGuiLayer::~VulkanGuiLayer() {}
    
    void VulkanGuiLayer::OnAttach() {
        BEAR_CORE_INFO("VulkanGuiLayer: Initializing...");
        
        // 获取Vulkan对象的引用
        Application& app = Application::Get();
        m_VulkanDevice = app.GetVulkanDevice();
        m_VulkanSurface = app.GetVulkanSurface();
        m_VulkanSwapchain = app.GetVulkanSwapchain();
        m_VulkanRenderPass = app.GetVulkanRenderPass();
		m_VulkanInstance = app.GetVulkanInstance();
        
        BEAR_CORE_ASSERT(m_VulkanDevice, "VulkanDevice is null!");
        BEAR_CORE_ASSERT(m_VulkanSurface, "VulkanSurface is null!");
        BEAR_CORE_ASSERT(m_VulkanSwapchain, "VulkanSwapchain is null!");
        BEAR_CORE_ASSERT(m_VulkanRenderPass, "VulkanRenderPass is null!");
        
        InitImGui();
        
        BEAR_CORE_INFO("VulkanGuiLayer: Initialized successfully");
    }
    
    void VulkanGuiLayer::OnDetach() {
        BEAR_CORE_INFO("VulkanGuiLayer: Shutting down...");
        
        // 等待设备空闲
        if (m_VulkanDevice) {
            vkDeviceWaitIdle(m_VulkanDevice->GetHandle());
        }
        
        ShutdownImGui();
        
        BEAR_CORE_INFO("VulkanGuiLayer: Shutdown complete");
    }
    
    void VulkanGuiLayer::InitImGui() {
        // 1. 创建ImGui上下文
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
        
        VkResult result = vkCreateDescriptorPool(m_VulkanDevice->GetHandle(), &pool_info, nullptr, &m_DescriptorPool);
        BEAR_CORE_ASSERT(result == VK_SUCCESS, "Failed to create descriptor pool for ImGui!");
        
        // 5. 创建命令池
        VkCommandPoolCreateInfo cmdPoolInfo = {};
        cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        //cmdPoolInfo.queueFamilyIndex = m_VulkanDevice->GetGraphicsQueue();
		cmdPoolInfo.queueFamilyIndex = m_VulkanDevice->GetQueueFamilyIndices().graphicsFamily.value();
        
        result = vkCreateCommandPool(m_VulkanDevice->GetHandle(), &cmdPoolInfo, nullptr, &m_CommandPool);
        BEAR_CORE_ASSERT(result == VK_SUCCESS, "Failed to create command pool for ImGui!");
        
        // 6. 创建命令缓冲区
        VkCommandBufferAllocateInfo cmdBufAllocInfo = {};
        cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufAllocInfo.commandPool = m_CommandPool;
        cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdBufAllocInfo.commandBufferCount = 1;
        
        result = vkAllocateCommandBuffers(m_VulkanDevice->GetHandle(), &cmdBufAllocInfo, &m_CommandBuffer);
        BEAR_CORE_ASSERT(result == VK_SUCCESS, "Failed to allocate command buffer for ImGui!");
        
        // 7. 创建同步对象
        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        
        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        
        vkCreateSemaphore(m_VulkanDevice->GetHandle(), &semaphoreInfo, nullptr, &m_ImageAvailableSemaphore);
        vkCreateSemaphore(m_VulkanDevice->GetHandle(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphore);
        vkCreateFence(m_VulkanDevice->GetHandle(), &fenceInfo, nullptr, &m_InFlightFence);
        
        // 8. 初始化ImGui Vulkan后端
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = m_VulkanInstance->GetHandle();
        init_info.PhysicalDevice = m_VulkanDevice->GetPhysicalDevice();
        init_info.Device = m_VulkanDevice->GetHandle();
        init_info.QueueFamily = m_VulkanDevice->GetQueueFamilyIndices().graphicsFamily.value();
        init_info.Queue = m_VulkanDevice->GetGraphicsQueue();
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.DescriptorPool = m_DescriptorPool;
        init_info.Subpass = 0;
        init_info.MinImageCount = m_VulkanSwapchain->GetImageCount();
        init_info.ImageCount = m_VulkanSwapchain->GetImageCount();
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator = nullptr;
        init_info.CheckVkResultFn = CheckVkResult;
        
        ImGui_ImplVulkan_Init(&init_info);
        
        // 9. 上传字体
        UploadFonts();
    }
    
    void VulkanGuiLayer::ShutdownImGui() {
        // 1. 销毁同步对象
        if (m_ImageAvailableSemaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_VulkanDevice->GetHandle(), m_ImageAvailableSemaphore, nullptr);
            m_ImageAvailableSemaphore = VK_NULL_HANDLE;
        }
        
        if (m_RenderFinishedSemaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_VulkanDevice->GetHandle(), m_RenderFinishedSemaphore, nullptr);
            m_RenderFinishedSemaphore = VK_NULL_HANDLE;
        }
        
        if (m_InFlightFence != VK_NULL_HANDLE) {
            vkDestroyFence(m_VulkanDevice->GetHandle(), m_InFlightFence, nullptr);
            m_InFlightFence = VK_NULL_HANDLE;
        }
        
        // 2. 销毁命令池（自动释放命令缓冲区）
        if (m_CommandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(m_VulkanDevice->GetHandle(), m_CommandPool, nullptr);
            m_CommandPool = VK_NULL_HANDLE;
        }
        
        // 3. 销毁描述符池
        if (m_DescriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(m_VulkanDevice->GetHandle(), m_DescriptorPool, nullptr);
            m_DescriptorPool = VK_NULL_HANDLE;
        }
        
        // 4. 关闭ImGui后端
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
    
    void VulkanGuiLayer::UploadFonts() {
        // 使用临时命令缓冲区上传字体
        VkCommandBuffer command_buffer = m_CommandBuffer;
        
        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        
        vkBeginCommandBuffer(command_buffer, &begin_info);
        ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
        vkEndCommandBuffer(command_buffer);
        
        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;
        
        vkQueueSubmit(m_VulkanDevice->GetGraphicsQueue(), 1, &submit_info, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_VulkanDevice->GetGraphicsQueue());
        
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }
    
    void VulkanGuiLayer::OnUpdate(float deltaTime) {
        // 等待上一帧完成
        vkWaitForFences(m_VulkanDevice->GetHandle(), 1, &m_InFlightFence, VK_TRUE, UINT64_MAX);
        
        // 获取下一个交换链图像
        uint32_t imageIndex;
        VkResult result = m_VulkanSwapchain->AcquireNextImage(m_ImageAvailableSemaphore, imageIndex);
        
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            m_SwapChainRebuild = true;
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            BEAR_CORE_ERROR("Failed to acquire swap chain image!");
            return;
        }
        
        // 重置栅栏
        vkResetFences(m_VulkanDevice->GetHandle(), 1, &m_InFlightFence);
        
        // 重置并开始记录命令缓冲区
        vkResetCommandBuffer(m_CommandBuffer, 0);
        
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        
        vkBeginCommandBuffer(m_CommandBuffer, &beginInfo);
        
        // 开始渲染通道
        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_VulkanRenderPass->GetHandle();
        renderPassInfo.framebuffer = m_VulkanSwapchain->GetFramebuffer(imageIndex);
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = m_VulkanSwapchain->GetExtent();
        
        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;
        
        vkCmdBeginRenderPass(m_CommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        
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
        ImDrawData* draw_data = ImGui::GetDrawData();
        ImGui_ImplVulkan_RenderDrawData(draw_data, m_CommandBuffer);
        
        // 结束渲染通道
        vkCmdEndRenderPass(m_CommandBuffer);
        
        // 结束命令缓冲区记录
        vkEndCommandBuffer(m_CommandBuffer);
        
        // 提交命令缓冲区
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        
        VkSemaphore waitSemaphores[] = {m_ImageAvailableSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_CommandBuffer;
        
        VkSemaphore signalSemaphores[] = {m_RenderFinishedSemaphore};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;
        
        result = vkQueueSubmit(m_VulkanDevice->GetGraphicsQueue(), 1, &submitInfo, m_InFlightFence);
        BEAR_CORE_ASSERT(result == VK_SUCCESS, "Failed to submit draw command buffer!");
        
        // 呈现结果
        result = m_VulkanSwapchain->Present(m_VulkanDevice->GetPresentQueue(), m_RenderFinishedSemaphore, imageIndex);
        
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_SwapChainRebuild) {
            m_SwapChainRebuild = false;
            // TODO: 重建交换链
        } else if (result != VK_SUCCESS) {
            BEAR_CORE_ERROR("Failed to present swap chain image!");
        }
        
        // 多视口支持
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }
    
    void VulkanGuiLayer::OnEvent(Event& event) {
        ImGuiIO& io = ImGui::GetIO();
        event.Handled |= event.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
        event.Handled |= event.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
    }
    
    void VulkanGuiLayer::CheckVkResult(VkResult err) {
        if (err == 0) return;
        BEAR_CORE_ERROR("Vulkan Error: VkResult = {}", err);
        if (err < 0) {
            BEAR_CORE_ASSERT(false, "Fatal Vulkan error!");
        }
    }
}