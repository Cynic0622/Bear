#include "bearpch.h"
#include "Sampler.h"
#include "Core/Device.h"

namespace Bear {
    Sampler::Sampler(const Device& device, const VkSamplerCreateInfo& createInfo)
		: m_Device(device)
	{
        BEAR_CORE_ASSERT(vkCreateSampler(m_Device.GetDevice(), &createInfo, nullptr, &m_Sampler) == VK_SUCCESS, "Failed to create texture sampler!");
    }

    Sampler::~Sampler() {
        if (m_Sampler != VK_NULL_HANDLE) {
            vkDestroySampler(m_Device.GetDevice(), m_Sampler, nullptr);
        }
    }
}