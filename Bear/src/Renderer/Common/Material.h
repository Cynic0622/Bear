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

	enum MaterialSlot : int {
		Params = 0, // Uniform buffer for material parameters
		BaseColor = 1,
		Normal = 2,
		MetallicRoughness = 3,
		Occlusion = 4,
		Emissive = 5,
		Count = 6
	};

	class Material {
	public:
		Material() = default;
		virtual ~Material() = default;

		Material(const Material&) = delete;
		Material& operator=(const Material&) = delete;

		virtual void UpdateParams() = 0;
		virtual void SetTexture(uint32_t binding, std::shared_ptr<Texture> texture) = 0;
		virtual void SetParam(const std::string& name, float value) = 0;
		virtual void SetParam(const std::string& name, const glm::vec3& value) = 0;
		virtual void SetParam(const std::string& name, const glm::vec4& value) = 0;
		virtual void SetTransparent(bool isTransparent) = 0;
		virtual bool IsTransparent() const = 0;

		// getters
		virtual RHIDescriptorSetLayout* GetDescriptorSetLayout() = 0;
		virtual RHIDescriptorSet* GetDescriptorSet() = 0;
	
	protected:
		const int MAX_FRAMES_IN_FLIGHT = 2;
	};
	using MaterialManager = ResourceManager<Material, std::string>;
}