#include "bearpch.h"

#include "PbrMaterial.h"

namespace Bear
{
	PbrMaterial::PbrMaterial(RHIDevice& device)
		:m_Device(device)
	{
		std::vector<RHIDescriptorSetLayoutBinding> bindings;
		// ubo
		bindings.push_back({ .binding = MaterialSlot::BaseColor, .descriptorType = DescriptorType::CombinedImageSampler, .stageFlags = ShaderStage::Fragment });
		bindings.push_back({ .binding = MaterialSlot::Normal, .descriptorType = DescriptorType::CombinedImageSampler, .stageFlags = ShaderStage::Fragment });
		bindings.push_back({ .binding = MaterialSlot::MetallicRoughness, .descriptorType = DescriptorType::CombinedImageSampler, .stageFlags = ShaderStage::Fragment });
		bindings.push_back({ .binding = MaterialSlot::Occlusion, .descriptorType = DescriptorType::CombinedImageSampler, .stageFlags = ShaderStage::Fragment });
		bindings.push_back({ .binding = MaterialSlot::Emissive, .descriptorType = DescriptorType::CombinedImageSampler, .stageFlags = ShaderStage::Fragment });

		m_DescriptorSetLayout = device.CreateDescriptorSetLayout(bindings);

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			m_DescriptorSets.push_back(device.CreateDescriptorSet(m_DescriptorSetLayout));
		}
	}
	PbrMaterial::~PbrMaterial()
	{
	}
	void PbrMaterial::UpdateParams(uint32_t currentFrame)
	{
		if (!m_ParamsDirty) return;
	}
	void PbrMaterial::SetTexture(uint32_t binding, std::shared_ptr<Texture> texture)
	{

	}
	void PbrMaterial::SetParam(const std::string& name, float value)
	{
		if (name == "metallicFactor")
		{
			m_Params.metallicFactor = value;
		}
		else if (name == "roughnessFactor")
		{
			m_Params.roughnessFactor = value;
		}
		else if (name == "normalScale")
		{
			m_Params.normalScale = value;
		}
		else if (name == "occlusionStrength")
		{
			m_Params.occlusionStrength = value;
		}
		else
		{
			BEAR_CORE_ERROR("Failed to set material param: " + name);
		}
		m_ParamsDirty = true; // need to update the uniform buffer
	}
	void PbrMaterial::SetParam(const std::string& name, const glm::vec3& value)
	{
		if (name == "emissiveFactor") m_Params.emissiveFactor = value;
		else
		{
			BEAR_CORE_ERROR("Failed to set material param: " + name);
		}
		m_ParamsDirty = true;
	}
	void PbrMaterial::SetParam(const std::string& name, const glm::vec4& value)
	{
		if (name == "baseColorFactor") m_Params.baseColorFactor = value;
		else
		{
			BEAR_CORE_ERROR("Failed to set material param: " + name);
		}
		m_ParamsDirty = true;
	}
}
