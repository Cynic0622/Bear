#pragma once
#include <vulkan/vulkan.h>

namespace Bear {
    class VulkanDevice;
    class VulkanSampler {
    public:
        VulkanSampler(const VulkanDevice& device);
        ~VulkanSampler();
        VkSampler GetHandle() const { return m_Sampler; }
    private:
        const VulkanDevice& m_Device;
        VkSampler m_Sampler = VK_NULL_HANDLE;
    };
}