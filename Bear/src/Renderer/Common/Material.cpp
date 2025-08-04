#include "bearpch.h"

#include "Pipeline/DescriptorSetLayout.h"
#include "Pipeline/Pipeline.h"
#include "Pipeline/DescriptorPool.h"
#include "Resources/Buffer.h"
#include "Material.h"
#include "Core/Device.h"
#include "Pipeline/RenderPass.h"
#include "Command/CommandBuffer.h"
#include "Resources/Image.h"
#include "Resources/Sampler.h"
#include "RHI/RHIDevice.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHITypes.h"
#include <memory>
namespace Bear {
	Material::Material(RHIDevice& device, const RHIRenderPass& renderPass, const std::vector<std::string>& shaderPath)
		:m_Device(device)
	{
		std::vector<RHIDescriptorSetLayoutBinding> bindings;
		bindings.push_back({ .binding = 0, .descriptorType = DescriptorType::UniformBuffer, .stageFlags = ShaderStage::Vertex });
		m_DescriptorSetLayout = device.CreateDescriptorSetLayout(bindings);
		m_PipelineLayout = device.CreatePipelineLayout({ m_DescriptorSetLayout });

		RHIPipelineConfig config{};
		config.pipelineLayout = m_PipelineLayout;
		config.vertexShaderPath = shaderPath[0];
		config.fragmentShaderPath = shaderPath[1];
		m_Pipeline = device.CreatePipeline(config, renderPass);

		m_UniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		m_DescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			m_UniformBuffers[i] = device.CreateBuffer(sizeof(UniformBufferObject), BufferUsage::UniformBuffer, true);
			m_DescriptorSets[i] = device.CreateDescriptorSet(m_DescriptorSetLayout);
			m_DescriptorSets[i]->UpdateDescriptorSet(0, *m_UniformBuffers[i]);
		}
	}
	Material::~Material()
	{
	}
	void Material::Bind(RHICommandList& commandBuffer, uint32_t currentFrame)
	{
		BEAR_CORE_ASSERT(m_Pipeline, "Pipeline is not created in Material!");
		BEAR_CORE_ASSERT(m_PipelineLayout, "Pipeline layout is not created in Material!");
		BEAR_CORE_ASSERT(m_DescriptorSets.size() > currentFrame, "Descriptor sets are not created in Material!");
		//BEAR_CORE_ASSERT(m_DescriptorSets[currentFrame].get() != nullptr, "Descriptor set is not created in Material!");
		auto& vkCommandBuffer = static_cast<CommandBuffer&>(commandBuffer);
		vkCommandBuffer.BindPipeline(*m_Pipeline);
		vkCommandBuffer.BindDescriptorSet(*m_PipelineLayout, *m_DescriptorSets[currentFrame]);
	}
	void Material::UpdateUniformBuffer(uint32_t currentFrame, const UniformBufferObject& ubo)
	{
		m_UniformBuffers[currentFrame]->UploadData(&ubo, sizeof(ubo));
	}
	//void Material::SetTexture(std::shared_ptr<Image> image, std::shared_ptr<Sampler> sampler)
	//{
	//	m_TextureImage = image;
	//	m_TextureSampler = sampler;
	//	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
	//		VkDescriptorImageInfo imageInfo{};
	//		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	//		imageInfo.imageView = m_TextureImage->GetView();
	//		imageInfo.sampler = m_TextureSampler->GetHandle();
	//		//m_DescriptorSets[i].UpdateDescriptorSets(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfo);
	//		VkWriteDescriptorSet descriptorWrite{};
	//		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	//		descriptorWrite.dstSet = m_DescriptorSets[i];
	//		descriptorWrite.dstBinding = 1;
	//		descriptorWrite.dstArrayElement = 0;
	//		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	//		descriptorWrite.descriptorCount = 1;
	//		descriptorWrite.pImageInfo = &imageInfo;

	//		vkUpdateDescriptorSets(m_Device.GetDevice(), 1, &descriptorWrite, 0, nullptr);
	//	}
	//}
	//void Material::CreateDescriptorSetLayout()
	//{
	//	uint32_t bindingCount = 2; // 0: Uniform Buffer, 1: Combined Image Sampler
	//	std::vector<VkDescriptorType> descriptorTypes = {
	//		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	//		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
	//	};
	//	std::vector<VkShaderStageFlags> stageFlags = {
	//		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, // Uniform Buffer
	//		VK_SHADER_STAGE_FRAGMENT_BIT // Combined Image Sampler
	//	};
	//	m_DescriptorSetLayout = std::make_unique<DescriptorSetLayout>(m_Device, bindingCount, descriptorTypes, stageFlags);
	//}
	//void Material::CreatePipelineLayout()
	//{
	//	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	//	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	//	VkDescriptorSetLayout setLayouts[] = { m_DescriptorSetLayout->GetHandle() };
	//	pipelineLayoutInfo.setLayoutCount = 1;
	//	pipelineLayoutInfo.pSetLayouts = setLayouts;

	//	BEAR_CORE_ASSERT(vkCreatePipelineLayout(m_Device.GetDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout) == VK_SUCCESS,
	//		"Failed to create pipeline layout in Material!");
	//}
	//void Material::CreatePipeline(const RenderPass& renderPass, const std::vector<std::string>& shaderPath)
	//{
	//	std::string vertPath = shaderPath[0];
	//	std::string fragPath = shaderPath[1];
	//	std::vector<std::unique_ptr<Shader>> shaders;
	//	shaders.push_back(std::make_unique<Shader>(m_Device, vertPath, VK_SHADER_STAGE_VERTEX_BIT));
	//	shaders.push_back(std::make_unique<Shader>(m_Device, fragPath, VK_SHADER_STAGE_FRAGMENT_BIT));

	//	PipelineConfigInfo configInfo{};
	//	PipelineConfigInfo::GetDefaultConfig(configInfo);
	//	configInfo.renderPass = renderPass.GetHandle();
	//	configInfo.pipelineLayout = m_PipelineLayout;

	//	m_Pipeline = std::make_unique<Pipeline>(m_Device, shaders, configInfo);
	//}
	//void Material::CreateUniformBuffers()
	//{
	//	m_UniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	//	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
	//		m_UniformBuffers[i] = std::make_unique<Buffer>(
	//			m_Device,
	//			sizeof(UniformBufferObject),
	//			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	//			VMA_MEMORY_USAGE_CPU_TO_GPU
	//		);
	//	}
	//}
	//void Material::CreateDescriptorPool()
	//{
	//	std::vector<VkDescriptorType> poolType = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER };
	//	m_DescriptorPool = std::make_unique<DescriptorPool>(m_Device, MAX_FRAMES_IN_FLIGHT, poolType.size(), poolType);
	//}
	//void Material::CreateDescriptorSets()
	//{
	//	m_DescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	//	VkDescriptorSetLayout setLayouts[] = { m_DescriptorSetLayout->GetHandle() };

	//	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
	//		//m_DescriptorSets[i] = VulkanDescriptorSets(m_Device, *m_DescriptorPool, 1, *m_DescriptorSetLayout);
	//		VkDescriptorSetAllocateInfo allocInfo{};
	//		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	//		allocInfo.descriptorPool = m_DescriptorPool->GetHandle();
	//		allocInfo.descriptorSetCount = 1;
	//		allocInfo.pSetLayouts = setLayouts;
	//		BEAR_CORE_ASSERT(vkAllocateDescriptorSets(m_Device.GetDevice(), &allocInfo, &m_DescriptorSets[i]) == VK_SUCCESS,
	//			"Failed to allocate descriptor set in Material!");

	//		VkDescriptorBufferInfo bufferInfo{};
	//		bufferInfo.buffer = m_UniformBuffers[i]->GetHandle();
	//		bufferInfo.offset = 0;
	//		bufferInfo.range = sizeof(UniformBufferObject);
	//		//m_DescriptorSets[i].UpdateDescriptorSets(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, *m_UniformBuffers[i]);
	//		VkWriteDescriptorSet descriptorWrite{};
	//		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	//		descriptorWrite.dstSet = m_DescriptorSets[i];
	//		descriptorWrite.dstBinding = 0;
	//		descriptorWrite.dstArrayElement = 0;
	//		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	//		descriptorWrite.descriptorCount = 1;
	//		descriptorWrite.pBufferInfo = &bufferInfo;

	//		vkUpdateDescriptorSets(m_Device.GetDevice(), 1, &descriptorWrite, 0, nullptr);
	//	}
	//}
}