#pragma once
#include "Material.h"

namespace Bear
{
	struct alignas(16) PbrMaterialParams {
		alignas(16) glm::vec4 baseColorFactor{ 1.f,1.f,1.f,1.f };
		float metallicFactor{ 1.f };
		float roughnessFactor{ 1.f };
		float normalScale{ 1.f };
		float occlusionStrength{ 1.f };
		alignas(16) glm::vec3 emissiveFactor{ 0.f,0.f,0.f };
		float _pad0{ 0.f }; // std140 alignment padding
	};

	class PbrMaterial : public Material
	{
	public:
		PbrMaterial(RHIDevice& device);
		~PbrMaterial() override;

		void UpdateParams() override;
		void SetTexture(uint32_t binding, std::shared_ptr<Texture> texture) override;
		void SetParam(const std::string& name, float value) override;
		void SetParam(const std::string& name, const glm::vec3& value) override;
		void SetParam(const std::string& name, const glm::vec4& value) override;
		void SetTransparent(bool isTransparent) override { m_IsTransparent = isTransparent; }
		// getters
		RHIDescriptorSetLayout* GetDescriptorSetLayout() override;
		RHIDescriptorSet* GetDescriptorSet() override;
		bool IsTransparent() const override { return m_IsTransparent; }
		static std::vector<RHIDescriptorSetLayoutBinding> GetDescriptorSetLayoutBinding();
	private:
		RHIDevice& m_Device;
		PbrMaterialParams m_Params;
		bool m_ParamsDirty = true;
		bool m_IsTransparent = false;
		std::unique_ptr<RHIDescriptorSet> m_DescriptorSets;
		std::shared_ptr<RHIDescriptorSetLayout> m_DescriptorSetLayout;
		std::shared_ptr<RHIBuffer> m_ParamsBuffer;
		std::array<std::shared_ptr<Texture>, MaterialSlot::Count> m_Textures;
		static std::vector<RHIDescriptorSetLayoutBinding> s_DescriptorSetLayoutBinding;
	};
}
