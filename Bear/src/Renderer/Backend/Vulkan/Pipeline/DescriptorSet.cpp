#include "bearpch.h"

#include "DescriptorSet.h"
#include "Device.h"
#include "DescriptorPool.h"
#include "DescriptorSetLayout.h"
#include "Image.h"
#include "Sampler.h"
#include "Resources/Buffer.h"
namespace Bear {
	DescriptorSet::DescriptorSet(const Device& device, const DescriptorSetLayout& layout, const DescriptorPool& descriptorPool)
		:m_Device(device), m_DescriptorPool(descriptorPool), m_SetLayout(layout)
	{
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPool.GetHandle();
		allocInfo.descriptorSetCount = 1;
		VkDescriptorSetLayout setLayout = m_SetLayout.GetHandle();
		allocInfo.pSetLayouts = &setLayout;
        
		BEAR_CORE_ASSERT(vkAllocateDescriptorSets(m_Device.GetDevice(), &allocInfo, &m_DescriptorSet) == VK_SUCCESS,
			"Failed to allocate descriptor set in Material!");
	}
	DescriptorSet::~DescriptorSet()
	{
		if (m_DescriptorSet != VK_NULL_HANDLE) {
			vkFreeDescriptorSets(m_Device.GetDevice(), m_DescriptorPool.GetHandle(), 1, &m_DescriptorSet);
			m_DescriptorSet = VK_NULL_HANDLE;
		}
	}
	void DescriptorSet::UpdateDescriptorSet(uint32_t dstBinding, const RHIBuffer& bufferInfo)
	{
		const auto& vkBuffer = static_cast<const Buffer&>(bufferInfo);

		VkDescriptorBufferInfo bufferInfoDesc{};
		bufferInfoDesc.buffer = vkBuffer.GetHandle();
		bufferInfoDesc.offset = 0;
		bufferInfoDesc.range = vkBuffer.GetSize();

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_DescriptorSet;
		descriptorWrite.dstBinding = dstBinding;
		descriptorWrite.dstArrayElement = 0;
		VkDescriptorType descriptorType = m_SetLayout.GetDescriptorType(dstBinding);
		descriptorWrite.descriptorType = descriptorType;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfoDesc;
		vkUpdateDescriptorSets(m_Device.GetDevice(), 1, &descriptorWrite, 0, nullptr);
	}
	void DescriptorSet::UpdateTexture(uint32_t binding, const RHIImage& image, const RHISampler& sampler)
	{
		const auto& vkImage = dynamic_cast<const Image&>(image);
		const auto& vkSampler = dynamic_cast<const Sampler&>(sampler);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageView = vkImage.GetView();
		// BEAR_CORE_INFO("UpdateTexture DescriptorSet={:#x}, binding {}: imageView={:#x}, sampler={:#x}",
		// 	(uint64_t)m_DescriptorSet, binding, (uint64_t)vkImage.GetView(), (uint64_t)vkSampler.GetHandle());

		if (vkImage.GetView() == VK_NULL_HANDLE) {
			BEAR_CORE_ERROR("UpdateTexture: imageView is VK_NULL_HANDLE for binding {}", binding);
			return;
		}
		imageInfo.sampler = vkSampler.GetHandle();
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_DescriptorSet;
		descriptorWrite.dstBinding = binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pImageInfo = &imageInfo;
		vkUpdateDescriptorSets(m_Device.GetDevice(), 1, &descriptorWrite, 0, nullptr);
	}
	void DescriptorSet::UpdateBuffer(uint32_t binding, const RHIBuffer& buffer)
	{
		// BEAR_CORE_INFO("UpdateBuffer DescriptorSet={:#x}, binding {}",
			// (uint64_t)m_DescriptorSet, binding);
		const auto& vkBuffer = dynamic_cast<const Buffer&>(buffer);
		VkDescriptorBufferInfo bufferInfoDesc{};
		bufferInfoDesc.buffer = vkBuffer.GetHandle();
		bufferInfoDesc.offset = 0;
		bufferInfoDesc.range = vkBuffer.GetSize();

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_DescriptorSet;
		descriptorWrite.dstBinding = binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = m_SetLayout.GetDescriptorType(binding);
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfoDesc;
		vkUpdateDescriptorSets(m_Device.GetDevice(), 1, &descriptorWrite, 0, nullptr);
	}
}