#pragma once
#include <vulkan/vulkan.h>

namespace Bear {
    class Device;
    class Sampler {
    public:
        Sampler(const Device& device);
        ~Sampler();
        VkSampler GetHandle() const { return m_Sampler; }
    private:
        const Device& m_Device;
        VkSampler m_Sampler = VK_NULL_HANDLE;
    };
}