#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "RHI/RHIPipeline.h"
#include "RHI/RHIResources.h"
namespace Bear {

	class VulkanDevice;
	class VulkanDescriptorPool;
	class VulkanDescriptorSetLayout;
	class VulkanBuffer;
	class VulkanDescriptorSet : public RHIDescriptorSet {
		// 改用单个资源管理
		//VulkanDescritorSet(const VulkanDevice& device, const VulkanDescriptorPool& descritorPool, uint32_t descriptorSetCount, const VulkanDescriptorSetLayout& setLayout);
	public:
		VulkanDescriptorSet(const VulkanDevice& device, const VulkanDescriptorSetLayout& layout, const VulkanDescriptorPool& descriptorPool);
		~VulkanDescriptorSet() override;

		VulkanDescriptorSet(const VulkanDescriptorSet&) = delete;
		VulkanDescriptorSet& operator=(const VulkanDescriptorSet&) = delete;

		void UpdateDescriptorSet(uint32_t dstBinding, const RHIBuffer& bufferInfo);

		VkDescriptorSet GetHandle() const { return m_DescriptorSet; }

		private:
			const VulkanDevice& m_Device;
			const VulkanDescriptorPool& m_DescriptorPool;
			const VulkanDescriptorSetLayout& m_SetLayout;
			VkDescriptorSet m_DescriptorSet;
	};
}