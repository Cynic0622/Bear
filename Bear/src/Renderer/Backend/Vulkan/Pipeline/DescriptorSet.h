#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "RHI/RHIPipeline.h"
#include "RHI/RHIResources.h"
namespace Bear {

	class Device;
	class DescriptorPool;
	class DescriptorSetLayout;
	class Buffer;
	class DescriptorSet : public RHIDescriptorSet {
		// 改用单个资源管理
		//VulkanDescritorSet(const Device& device, const DescriptorPool& descritorPool, uint32_t descriptorSetCount, const DescriptorSetLayout& setLayout);
	public:
		DescriptorSet(const Device& device, const DescriptorSetLayout& layout, const DescriptorPool& descriptorPool);
		~DescriptorSet() override;

		DescriptorSet(const DescriptorSet&) = delete;
		DescriptorSet& operator=(const DescriptorSet&) = delete;

		void UpdateDescriptorSet(uint32_t dstBinding, const RHIBuffer& bufferInfo);

		VkDescriptorSet GetHandle() const { return m_DescriptorSet; }

		private:
			const Device& m_Device;
			const DescriptorPool& m_DescriptorPool;
			const DescriptorSetLayout& m_SetLayout;
			VkDescriptorSet m_DescriptorSet;
	};
}