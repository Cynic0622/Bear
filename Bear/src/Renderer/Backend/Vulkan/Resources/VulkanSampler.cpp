#include "bearpch.h"
#include "VulkanSampler.h"
#include "Core/VulkanDevice.h"
#include "Core/VulkanUtils.h"

namespace Bear {
    VulkanSampler::VulkanSampler(const VulkanDevice& device) : m_Device(device) {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(m_Device.GetPhysicalDevice(), &properties);
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        BEAR_CORE_ASSERT(vkCreateSampler(m_Device.GetDevice(), &samplerInfo, nullptr, &m_Sampler) == VK_SUCCESS, "Failed to create texture sampler!");
    }

    VulkanSampler::~VulkanSampler() {
        if (m_Sampler != VK_NULL_HANDLE) {
            vkDestroySampler(m_Device.GetDevice(), m_Sampler, nullptr);
        }
    }
}