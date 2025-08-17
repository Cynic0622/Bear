#pragma once
#include <vulkan/vulkan.h>
#include "RHI/RHIPipeline.h"
namespace Bear{

	class Device;
    class DescriptorSetLayout : public RHIDescriptorSetLayout {
    public:
        // ÷ÿππ
        /*DescriptorSetLayout(const Device& device, uint32_t bondingCount, const std::vector<VkDescriptorType>& descriptorTypes, 
            std::vector<VkShaderStageFlags> stageFlags);*/
        DescriptorSetLayout(const Device& device, const std::vector<VkDescriptorSetLayoutBinding>& bindings);
        ~DescriptorSetLayout() override;

        DescriptorSetLayout(const DescriptorSetLayout&) = delete;
        DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

		VkDescriptorType GetDescriptorType(uint32_t binding) const;

        VkDescriptorSetLayout GetHandle() const { return m_Layout; }

    private:
        const Device& m_Device;
        VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
		std::vector<VkDescriptorType> m_BindingTypes;
    };
}