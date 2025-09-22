#pragma once
#include "Material.h"
#include "libntc/ntc.h"

namespace Bear
{
	class NtcMaterial : public Material
	{
	public:
		NtcMaterial(RHIDevice& device);
		~NtcMaterial() override = default;
		RHIDescriptorSet* GetDescriptorSet() override { return m_DescriptorSets.get(); }
		RHIDescriptorSetLayout* GetDescriptorSetLayout() override { return m_DescriptorSetLayout.get(); }
		void SetBuffer(NtcMaterialSlot slot, size_t dataSize, const void* data);
	public:
		std::shared_ptr<RHIBuffer> m_ConstantBuffer;
		std::shared_ptr<RHIBuffer> m_LatentBuffer;
		std::shared_ptr<RHIBuffer> m_WeightBuffer;
		ntc::StreamRange latentStreamRange;

		int networkVersion = 0;
		int weightType = 0;
		size_t memorySize = 0;
	private:
		RHIDevice& m_Device;
		std::shared_ptr<RHIDescriptorSetLayout> m_DescriptorSetLayout;
		std::unique_ptr<RHIDescriptorSet> m_DescriptorSets;
	};
}
