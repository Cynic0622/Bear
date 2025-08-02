#pragma once
#include <vulkan/vulkan.h>
#include "RHI/RHIPipeline.h"
namespace Bear{

	class VulkanDevice;
    class VulkanDescriptorSetLayout : public RHIDescriptorSetLayout {
    public:
        // ÷ÿππ
        /*VulkanDescriptorSetLayout(const VulkanDevice& device, uint32_t bondingCount, const std::vector<VkDescriptorType>& descriptorTypes, 
            std::vector<VkShaderStageFlags> stageFlags);*/
        VulkanDescriptorSetLayout(const VulkanDevice& device, const std::vector<VkDescriptorSetLayoutBinding>& bindings);
        ~VulkanDescriptorSetLayout() override;

        VulkanDescriptorSetLayout(const VulkanDescriptorSetLayout&) = delete;
        VulkanDescriptorSetLayout& operator=(const VulkanDescriptorSetLayout&) = delete;

		VkDescriptorType GetDescriptorType(uint32_t binding) const;

        VkDescriptorSetLayout GetHandle() const { return m_Layout; }

    private:
        const VulkanDevice& m_Device;
        VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
		std::vector<VkDescriptorType> m_BindingTypes;
    };
}