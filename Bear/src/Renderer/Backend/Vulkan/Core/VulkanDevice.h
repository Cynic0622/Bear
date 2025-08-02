#pragma once
#include <vulkan/vulkan.h>
#include "VulkanTypes.h"
#include <vector>
#include "vk_mem_alloc.h" // vma
#include "RHI/RHIDevice.h"

namespace Bear {
	class VulkanInstance;
	class VulkanSurface;
	class VulkanDescriptorPool;
	struct RHIAttachmentDescription;
    class VulkanDevice : public RHIDevice {
    public:
        VulkanDevice(const VulkanInstance& instance, const VulkanSurface& surface);
        ~VulkanDevice();

        // 禁止拷贝和移动
        VulkanDevice(const VulkanDevice&) = delete;
        VulkanDevice& operator=(const VulkanDevice&) = delete;
        VulkanDevice(VulkanDevice&&) = delete;
        VulkanDevice& operator=(VulkanDevice&&) = delete;
 
        inline VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
        inline VkDevice GetDevice() const { return m_LogicalDevice; }
        inline VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
        inline VkQueue GetPresentQueue() const { return m_PresentQueue; }
		inline const VulkanSurface& GetSurface() const { return m_Surface; }
        inline const QueueFamilyIndices& GetQueueFamilyIndices() const { return m_QueueIndices; }
		inline VmaAllocator GetAllocator() const { return m_Allocator; }

        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
		inline void WaitIdle() const { vkDeviceWaitIdle(m_LogicalDevice); }

		// 实现 RHIDevice 接口
		std::unique_ptr<RHIBuffer> CreateBuffer(size_t size, BufferUsage usage, bool cpuAccessible) override;
        std::shared_ptr<RHIDescriptorSetLayout> CreateDescriptorSetLayout(const std::vector<RHIDescriptorSetLayoutBinding>& bindings) override;
		std::shared_ptr<RHIPipelineLayout> CreatePipelineLayout(const std::vector<std::shared_ptr<RHIDescriptorSetLayout>>& descriptorSetLayouts) override;
        std::shared_ptr<RHIPipeline> CreatePipeline(const RHIPipelineConfig& config, const RHIRenderPass& renderPass) override;
        std::unique_ptr<RHIDescriptorSet> CreateDescriptorSet(std::shared_ptr<RHIDescriptorSetLayout> layout) override;
        std::shared_ptr<RHIRenderPass> CreateRenderPass(const std::vector <RHIAttachmentDescription>& attachments) override;
        std::unique_ptr<RHISwapchain> CreateSwapchain(std::shared_ptr<RHIRenderPass> renderPass) override;
        

    private:
		const VulkanSurface& m_Surface;
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
        std::unique_ptr<VulkanDescriptorPool> m_GlobalDescriptorPool;

    private:
        void CreateLogicalDevice(VkInstance instance, VkSurfaceKHR surface);
        void PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
        bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
		bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
    };
}