#include "bearpch.h"

#include "DescriptorSetLayout.h"

#include "Device.h"

namespace Bear {
	DescriptorSetLayout::DescriptorSetLayout(const Device& device, const std::vector<VkDescriptorSetLayoutBinding>& bindings)
		:m_Device(device)
	{
		// 7/29/2025
		//std::vector<VkDescriptorSetLayoutBinding> bindings(bindings.size());
		//for (uint32_t i = 0; i < bondingCount; ++i) {
		//	VkDescriptorSetLayoutBinding binding = {};
		//	binding.binding = i;
		//	binding.descriptorType = descriptorTypes[i];
		//	binding.descriptorCount = 1;
		//	binding.stageFlags = stageFlags[i];
		//	binding.pImmutableSamplers = nullptr; // Optional, can be set if using samplers
		//	bindings[i] = binding;
		//}
		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = bindings.size();
		layoutInfo.pBindings = bindings.data();
		BEAR_CORE_ASSERT(vkCreateDescriptorSetLayout(m_Device.GetDevice(), &layoutInfo, nullptr, &m_Layout) == VK_SUCCESS, "failed to create descriptor set layout!");
		m_BindingTypes.resize(bindings.size());
		for (const auto& binding : bindings) {
			m_BindingTypes[binding.binding] = binding.descriptorType;
		}

		/**
		* 7/27/2025
		* 重构构造函数的参数，允许用户指定绑定数量、描述符类型和着色器阶段标志
		*/
		//VkDescriptorSetLayoutBinding uboLayoutBinding = {};
		//uboLayoutBinding.binding = 0;
		//uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		//uboLayoutBinding.descriptorCount = 1;
		//uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		//uboLayoutBinding.pImmutableSamplers = nullptr; // Optional
		//VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		//layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		//layoutInfo.bindingCount = 1;
		//layoutInfo.pBindings = &uboLayoutBinding;
		//BEAR_CORE_ASSERT(vkCreateDescriptorSetLayout(m_Device.GetDevice(), &layoutInfo, nullptr, &m_Layout) == VK_SUCCESS, "failed to create descriptor set layout!")
	}
	DescriptorSetLayout::~DescriptorSetLayout()
	{
		if (m_Layout != VK_NULL_HANDLE) {
			vkDestroyDescriptorSetLayout(m_Device.GetDevice(), m_Layout, nullptr);
			m_Layout = VK_NULL_HANDLE;
		}
	}
	VkDescriptorType DescriptorSetLayout::GetDescriptorType(uint32_t binding) const
	{
		// 查询绑定的描述符类型
		return m_BindingTypes[binding];
	}
}