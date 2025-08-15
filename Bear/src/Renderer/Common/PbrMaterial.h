#pragma once
#include "Material.h"

namespace Bear
{
	struct alignas(16) MaterialParams {
		alignas(16) glm::vec4 baseColorFactor{ 1.f,1.f,1.f,1.f };
		alignas(4)  float metallicFactor{ 1.f };
		alignas(4)  float roughnessFactor{ 1.f };
		alignas(4)  float normalScale{ 1.f };
		alignas(4)  float occlusionStrength{ 1.f };
		alignas(16) glm::vec3 emissiveFactor{ 0.f,0.f,0.f };
		float _pad0{ 0.f }; // std140 ¶ÔÆëÌî³ä
	};

	class PbrMaterial : public Material
	{
	public:
		PbrMaterial(RHIDevice& device);
		~PbrMaterial() override;

		void UpdateParams(uint32_t currentFrame) override;
		void SetTexture(uint32_t binding, std::shared_ptr<Texture> texture) override;
		void SetParam(const std::string& name, float value) override;
		void SetParam(const std::string& name, const glm::vec3& value) override;
		void SetParam(const std::string& name, const glm::vec4& value) override;

	private:
		RHIDevice& m_Device;
		MaterialParams m_Params;
		bool m_ParamsDirty = true;
		const int MAX_FRAMES_IN_FLIGHT = 2;
		std::vector<std::unique_ptr<RHIDescriptorSet>> m_DescriptorSets;
		std::shared_ptr<RHIDescriptorSetLayout> m_DescriptorSetLayout;
	};
}
