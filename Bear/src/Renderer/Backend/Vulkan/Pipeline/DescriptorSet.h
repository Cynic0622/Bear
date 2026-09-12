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
		// ���õ�����Դ����
		//VulkanDescritorSet(const Device& device, const DescriptorPool& descritorPool, uint32_t descriptorSetCount, const DescriptorSetLayout& setLayout);
	public:
		DescriptorSet(const Device& device, const DescriptorSetLayout& layout, const DescriptorPool& descriptorPool);
		~DescriptorSet() override;

		DescriptorSet(const DescriptorSet&) = delete;
		DescriptorSet& operator=(const DescriptorSet&) = delete;

		void UpdateDescriptorSet(uint32_t dstBinding, const RHIBuffer& bufferInfo) override;
		void UpdateTexture(uint32_t binding, const RHIImage& image, const RHISampler& sampler, uint32_t mipLevel = UINT32_MAX) override;
		void UpdateSampledImage(uint32_t binding, const RHIImage& image, const RHISampler& sampler, uint32_t mipLevel, ImageLayout layout) override;
		void UpdateStorageImage(uint32_t binding, const RHIImage& image, uint32_t mipLevel) override;
		void UpdateBuffer(uint32_t binding, const RHIBuffer& buffer) override;

		VkDescriptorSet GetHandle() const { return m_DescriptorSet; }

		private:
			const Device& m_Device;
			const DescriptorPool& m_DescriptorPool;
			const DescriptorSetLayout& m_SetLayout;
			VkDescriptorSet m_DescriptorSet;
	};
}