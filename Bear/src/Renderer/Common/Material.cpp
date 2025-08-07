#include "bearpch.h"

#include "Pipeline/Pipeline.h"
#include "Material.h"
#include "Core/Device.h"
#include "Command/CommandBuffer.h"
#include "Resources/Image.h"
#include "RHI/RHIDevice.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHITypes.h"
#include <memory>

#include "Texture.h"

namespace Bear {
	Material::Material(RHIDevice& device, const RHIRenderPass& renderPass, const std::vector<std::string>& shaderPath)
		:m_Device(device)
	{
		std::vector<RHIDescriptorSetLayoutBinding> bindings;
		// ubo
		bindings.push_back({ .binding = 0, .descriptorType = DescriptorType::UniformBuffer, .stageFlags = ShaderStage::Vertex });
		// texture
		bindings.push_back({ .binding = 1, .descriptorType = DescriptorType::CombinedImageSampler, .stageFlags = ShaderStage::Fragment });

		m_DescriptorSetLayout = device.CreateDescriptorSetLayout(bindings);
		std::vector<RHIPushConstantRange> pushConstantRanges;
		pushConstantRanges.push_back({ .stageFlags = ShaderStage::Vertex, .offset = 0, .size = sizeof(UniformBufferObject) });
		m_PipelineLayout = device.CreatePipelineLayout({ m_DescriptorSetLayout.get() }, pushConstantRanges);

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
	void Material::Bind(RHICommandList& commandBuffer, uint32_t currentFrame) const
	{
		BEAR_CORE_ASSERT(m_Pipeline, "Pipeline is not created in Material!")
		BEAR_CORE_ASSERT(m_PipelineLayout, "Pipeline layout is not created in Material!")
		BEAR_CORE_ASSERT(m_DescriptorSets.size() > currentFrame, "Descriptor sets are not created in Material!")

		auto& vkCommandBuffer = dynamic_cast<CommandBuffer&>(commandBuffer);
		vkCommandBuffer.BindPipeline(*m_Pipeline);
		vkCommandBuffer.BindDescriptorSet(*m_PipelineLayout, *m_DescriptorSets[currentFrame]);
	}
	void Material::UpdateUniformBuffer(uint32_t currentFrame, const UniformBufferObject& ubo) const
	{
		m_UniformBuffers[currentFrame]->UploadData(&ubo, sizeof(ubo));
	}
	void Material::SetTexture(uint32_t binding, std::shared_ptr<Texture> texture)
	{
		m_Texture = std::move(texture);
		if (m_Texture)
		{
			for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
			{
				m_DescriptorSets[i]->UpdateTexture(binding, m_Texture->GetImage(), m_Texture->GetSampler());
			}
		}
	}
}
