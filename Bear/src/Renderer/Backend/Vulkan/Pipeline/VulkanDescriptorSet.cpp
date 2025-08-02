#include "bearpch.h"

#include "VulkanDescriptorSet.h"
#include "VulkanDevice.h"
#include "VulkanDescriptorPool.h"
#include "VulkanDescriptorSetLayout.h"
#include "Resources/VulkanBuffer.h"
namespace Bear {
	VulkanDescriptorSet::VulkanDescriptorSet(const VulkanDevice& device, const VulkanDescriptorSetLayout& layout, const VulkanDescriptorPool& descriptorPool)
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
	VulkanDescriptorSet::~VulkanDescriptorSet()
	{
		if (m_DescriptorSet != VK_NULL_HANDLE) {
			vkFreeDescriptorSets(m_Device.GetDevice(), m_DescriptorPool.GetHandle(), 1, &m_DescriptorSet);
			m_DescriptorSet = VK_NULL_HANDLE;
		}
	}
	void VulkanDescriptorSet::UpdateDescriptorSet(uint32_t dstBinding, const RHIBuffer& bufferInfo)
	{
		const auto& vkBuffer = static_cast<const VulkanBuffer&>(bufferInfo);

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
}