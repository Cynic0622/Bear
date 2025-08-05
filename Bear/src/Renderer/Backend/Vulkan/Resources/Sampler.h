#pragma once
#include <vulkan/vulkan.h>

#include "RHI/RHIResources.h"

namespace Bear {
    class Device;
    class Sampler : public RHISampler{
    public:
        Sampler(const Device& device, const VkSamplerCreateInfo& createInfo);
        ~Sampler();
        VkSampler GetHandle() const { return m_Sampler; }
    private:
        const Device& m_Device;
        VkSampler m_Sampler = VK_NULL_HANDLE;
    };
}
