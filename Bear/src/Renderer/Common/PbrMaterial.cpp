#include "bearpch.h"

#include "PbrMaterial.h"

#include "Texture.h"

namespace Bear
{
	std::vector<RHIDescriptorSetLayoutBinding> PbrMaterial::s_DescriptorSetLayoutBinding;
	PbrMaterial::PbrMaterial(RHIDevice& device)
		:m_Device(device)
	{
		m_ParamsBuffer = m_Device.CreateBuffer(sizeof(PbrMaterialParams), BufferUsage::UniformBuffer, true);
		
		std::vector<RHIDescriptorSetLayoutBinding> bindings = GetDescriptorSetLayoutBinding();
		m_DescriptorSetLayout = m_Device.CreateDescriptorSetLayout(bindings);

		m_DescriptorSets = device.CreateDescriptorSet(m_DescriptorSetLayout);
		m_DescriptorSets->UpdateBuffer(MaterialSlot::Params, *m_ParamsBuffer);
	}
	PbrMaterial::~PbrMaterial()
	{
		// use smart pointers to automatically release resources.
	}
	void PbrMaterial::UpdateParams()
	{
		// if (!m_ParamsDirty) return;
		m_ParamsBuffer->UploadData(&m_Params, sizeof(PbrMaterialParams));

	}
	void PbrMaterial::SetTexture(uint32_t binding, std::shared_ptr<Texture> texture)
	{
		BEAR_CORE_ASSERT(binding < MaterialSlot::Count, "Invalid material slot binding: " + std::to_string(binding));
		m_DescriptorSets->UpdateTexture(binding, texture->GetImage(), texture->GetSampler());
		m_Textures[binding] = std::move(texture); // store the texture to keep it alive.
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
		UpdateParams();
	}
	void PbrMaterial::SetParam(const std::string& name, const glm::vec3& value)
	{
		if (name == "emissiveFactor") m_Params.emissiveFactor = value;
		else
		{
			BEAR_CORE_ERROR("Failed to set material param: " + name);
		}
		m_ParamsDirty = true;
		UpdateParams();
	}
	void PbrMaterial::SetParam(const std::string& name, const glm::vec4& value)
	{
		if (name == "baseColorFactor") m_Params.baseColorFactor = value;
		else
		{
			BEAR_CORE_ERROR("Failed to set material param: " + name);
		}
		m_ParamsDirty = true;
		UpdateParams();
	}
	RHIDescriptorSetLayout* PbrMaterial::GetDescriptorSetLayout()
	{
		return m_DescriptorSetLayout.get();
	}
	RHIDescriptorSet* PbrMaterial::GetDescriptorSet()
	{
		return m_DescriptorSets.get();
	}
	std::vector<RHIDescriptorSetLayoutBinding> PbrMaterial::GetDescriptorSetLayoutBinding()
	{
		if (s_DescriptorSetLayoutBinding.empty())
		{
			// ubo
			s_DescriptorSetLayoutBinding.push_back({ .binding = MaterialSlot::Params, .descriptorType = DescriptorType::UniformBuffer, .stageFlags = ShaderStage::Vertex | ShaderStage::Fragment });
			// pbr textures
			s_DescriptorSetLayoutBinding.push_back({ .binding = MaterialSlot::BaseColor, .descriptorType = DescriptorType::CombinedImageSampler, .stageFlags = ShaderStage::Fragment });
			s_DescriptorSetLayoutBinding.push_back({ .binding = MaterialSlot::Normal, .descriptorType = DescriptorType::CombinedImageSampler, .stageFlags = ShaderStage::Fragment });
			s_DescriptorSetLayoutBinding.push_back({ .binding = MaterialSlot::MetallicRoughness, .descriptorType = DescriptorType::CombinedImageSampler, .stageFlags = ShaderStage::Fragment });
			s_DescriptorSetLayoutBinding.push_back({ .binding = MaterialSlot::Occlusion, .descriptorType = DescriptorType::CombinedImageSampler, .stageFlags = ShaderStage::Fragment });
			s_DescriptorSetLayoutBinding.push_back({ .binding = MaterialSlot::Emissive, .descriptorType = DescriptorType::CombinedImageSampler, .stageFlags = ShaderStage::Fragment });
		}
		return s_DescriptorSetLayoutBinding;
	}
}
