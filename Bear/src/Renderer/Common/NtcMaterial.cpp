#include "bearpch.h"
#include "NtcMaterial.h"

namespace Bear
{
	NtcMaterial::NtcMaterial(RHIDevice& device)
		:m_Device(device)
	{
		std::vector<RHIDescriptorSetLayoutBinding> bindings;
		bindings.push_back({ .binding = NtcMaterialSlot::Latent, .descriptorType = DescriptorType::UniformBuffer, .stageFlags = ShaderStage::Fragment });
		bindings.push_back({ .binding = NtcMaterialSlot::Weight, .descriptorType = DescriptorType::UniformBuffer, .stageFlags = ShaderStage::Fragment });
		bindings.push_back({ .binding = NtcMaterialSlot::Constant, .descriptorType = DescriptorType::UniformBuffer, .stageFlags = ShaderStage::Fragment });
		m_DescriptorSetLayout = device.CreateDescriptorSetLayout(bindings);
		m_DescriptorSets = device.CreateDescriptorSet(m_DescriptorSetLayout);
	}
	void NtcMaterial::SetBuffer(NtcMaterialSlot slot, size_t dataSize, const void* data)
	{
		switch (slot)
		{
		case NtcMaterialSlot::Latent:
			m_LatentBuffer = m_Device.CreateBuffer(dataSize, BufferUsage::UniformBuffer, true);
			m_DescriptorSets->UpdateBuffer(NtcMaterialSlot::Latent, *m_LatentBuffer);
			m_LatentBuffer->UploadData(data, dataSize);
			latentStreamRange = { 0, dataSize };
			break;
		case NtcMaterialSlot::Weight:
			m_WeightBuffer = m_Device.CreateBuffer(dataSize, BufferUsage::UniformBuffer, true);
			m_DescriptorSets->UpdateBuffer(NtcMaterialSlot::Weight, *m_WeightBuffer);
			m_WeightBuffer->UploadData(data, dataSize);
			break;
		case NtcMaterialSlot::Constant:
			m_ConstantBuffer = m_Device.CreateBuffer(dataSize, BufferUsage::UniformBuffer, true);
			m_DescriptorSets->UpdateBuffer(NtcMaterialSlot::Constant, *m_ConstantBuffer);
			m_ConstantBuffer->UploadData(data, dataSize);
			break;
		default:
			BEAR_CORE_ERROR("Invalid NTC material slot");
			break;
		}
	}
}

