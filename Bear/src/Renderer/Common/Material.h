#pragma once
#include <string>

#include "ResourceManager.h"

namespace Bear {
	class Texture;

	class RenderPass;
	struct UniformBufferObject;
	class RHIDescriptorSetLayout;
	class RHIPipeline;
	class RHIBuffer;
	class Image;
	class Sampler;
	class RHIDescriptorSets;
	class RHIDevice;
	class RHIRenderPass;

	enum MaterialSlot : uint8_t {
		Params = 0, // Uniform buffer for material parameters
		BaseColor = 1,
		Normal = 2,
		MetallicRoughness = 3,
		Occlusion = 4,
		Emissive = 5,
		Count = 6
	};

	// ntc
	enum NtcMaterialSlot : uint8_t {
		Latent = 0,
		Weight = 1,
		Constant = 2,
		NtcCount = 3
	};

	class Material {
	public:
		Material() = default;
		virtual ~Material() = default;

		Material(const Material&) = delete;
		Material& operator=(const Material&) = delete;

		virtual void UpdateParams() {}
		virtual void SetTexture(uint32_t binding, std::shared_ptr<Texture> texture) {}
		virtual void SetParam(const std::string& name, float value) {}
		virtual void SetParam(const std::string& name, const glm::vec3& value) {}
		virtual void SetParam(const std::string& name, const glm::vec4& value) {}
		virtual void SetTransparent(bool isTransparent) {}
		virtual bool IsTransparent() const { return false; }

		// getters
		virtual RHIDescriptorSetLayout* GetDescriptorSetLayout() { return nullptr; }
		virtual RHIDescriptorSet* GetDescriptorSet() { return nullptr; }
	
	protected:
		const int MAX_FRAMES_IN_FLIGHT = 2;
	};
	using MaterialManager = ResourceManager<Material, std::string>;
}