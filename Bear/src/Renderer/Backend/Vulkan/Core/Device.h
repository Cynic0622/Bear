#pragma once
#include <vulkan/vulkan.h>
#include "Types.h"
#include <vector>
#include "vk_mem_alloc.h" // vma
#include "RHI/RHIDevice.h"

namespace Bear {
	class Instance;
	class Surface;
	class DescriptorPool;
	struct RHIAttachmentDescription;
	class CommandPool;
	class CommandBuffer;
	class Semaphore;
	class Fence;
	class Swapchain;
    class Device : public RHIDevice {
    public:
        Device(GLFWwindow* window);
        ~Device() override;

        // 禁止拷贝和移动
        Device(const Device&) = delete;
        Device& operator=(const Device&) = delete;
        Device(Device&&) = delete;
        Device& operator=(Device&&) = delete;
 
        inline VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
        inline VkDevice GetDevice() const { return m_LogicalDevice; }
        inline VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
        inline VkQueue GetPresentQueue() const { return m_PresentQueue; }
		inline const Surface& GetSurface() const { return *m_Surface; }
        inline const QueueFamilyIndices& GetQueueFamilyIndices() const { return m_QueueIndices; }
		inline VmaAllocator GetAllocator() const { return m_Allocator; }
        void Present(RHISwapchain& swapchain, uint32_t imageIndex);
		// 实现 RHIDevice 接口
        void SubmitCommands(RHICommandList& cmd);
		std::unique_ptr<RHIBuffer> CreateBuffer(size_t size, BufferUsage usage, bool cpuAccessible) override;
        std::shared_ptr<RHIDescriptorSetLayout> CreateDescriptorSetLayout(const std::vector<RHIDescriptorSetLayoutBinding>& bindings) override;
		std::shared_ptr<RHIPipelineLayout> CreatePipelineLayout(const std::vector<std::shared_ptr<RHIDescriptorSetLayout>>& descriptorSetLayouts) override;
        std::shared_ptr<RHIPipeline> CreatePipeline(const RHIPipelineConfig& config, const RHIRenderPass& renderPass) override;
        std::unique_ptr<RHIDescriptorSet> CreateDescriptorSet(std::shared_ptr<RHIDescriptorSetLayout> layout) override;
        std::shared_ptr<RHIRenderPass> CreateRenderPass(const std::vector <RHIAttachmentDescription>& attachments) override;
        std::unique_ptr<RHISwapchain> CreateSwapchain(RHIRenderPass& renderPass) override;
		RHICommandList& BeginFrame() override;
		void EndFrame(RHISwapchain& swapchain, uint32_t imageIndex) override;
		uint32_t GetCurrentFrameIndex() const override { return m_CurrentFrame; }
		uint32_t AcquireNextImage(RHISwapchain& swapchain) const override;
		
        inline void WaitIdle() override { vkDeviceWaitIdle(m_LogicalDevice); }
        

    private:
        std::unique_ptr<Instance> m_Instance;
        std::unique_ptr<Surface> m_Surface;
		
		uint32_t m_CurrentFrame = 0; // 当前帧索引，用于双缓冲或三缓冲
		uint32_t m_CurrentImageIndex = 0; // 当前图像索引
        
        std::unique_ptr<CommandPool> m_CommandPool;
        std::vector<std::unique_ptr<CommandBuffer>> m_CommandBuffers;

        const int MAX_FRAMES_IN_FLIGHT = 2;

        std::vector<std::unique_ptr<Semaphore>> m_ImageAvailableSemaphores;
        std::vector<std::unique_ptr<Semaphore>> m_RenderFinishedSemaphores;
        std::vector<std::unique_ptr<Fence>> m_InFlightFences;

        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkDevice m_LogicalDevice = VK_NULL_HANDLE;

		VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
        VkQueue m_PresentQueue = VK_NULL_HANDLE;

        QueueFamilyIndices m_QueueIndices;

        const std::vector<const char*> m_DeviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            /*VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
            VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME*/
        };

		VmaAllocator m_Allocator; // Vulkan Memory Allocator
        std::unique_ptr<DescriptorPool> m_GlobalDescriptorPool;

    private:
        void CreateLogicalDevice(VkInstance instance, VkSurfaceKHR surface);
        void PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
        bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
		bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
    };
}