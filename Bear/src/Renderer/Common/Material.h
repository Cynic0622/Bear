#pragma once
#include <string>
#include <vector>

#include "ResourceManager.h"
#include "RHI/RHICommandList.h"
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
		BaseColor = 1,
		Normal = 2,
		MetallicRoughness = 3,
		Occlusion = 4,
		Emissive = 5
	};

	class Material {
	public:
		Material() = default;
		// Material(RHIDevice& device, const RHIRenderPass& renderPass, const std::vector<std::string>& shaderPath);
		virtual ~Material();

		Material(const Material&) = delete;
		Material& operator=(const Material&) = delete;

		void Bind(RHICommandList& commandBuffer, uint32_t currentFrame) const;

		// void UpdateUniformBuffer(uint32_t currentFrame, const MaterialParams& params);
		virtual void UpdateParams(uint32_t currentFrame);
		virtual void SetTexture(uint32_t binding, std::shared_ptr<Texture> texture);
		virtual void SetParam(const std::string& name, float value);
		virtual void SetParam(const std::string& name, const glm::vec3& value);
		virtual void SetParam(const std::string& name, const glm::vec4& value);

		RHIPipelineLayout* GetPipelineLayout() const { return m_PipelineLayout.get(); }
	private:
		// const RHIDevice& m_Device;
		const int MAX_FRAMES_IN_FLIGHT = 2; // 与 Renderer 保持一致

		// 材质拥有的渲染状态
		// 考虑到一些可以共享的资源，这里改为shared_ptr
		std::shared_ptr<RHIDescriptorSetLayout> m_DescriptorSetLayout;
		std::shared_ptr<RHIPipelineLayout> m_PipelineLayout;
		//VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
		std::shared_ptr<RHIPipeline> m_Pipeline;

		// 描述符资源 (每个 in-flight frame 一个)
		//std::unique_ptr<DescriptorPool> m_DescriptorPool; // 有了全局的pool，这里就不需要了
		std::vector<std::unique_ptr<RHIBuffer>> m_UniformBuffers;
		std::vector<std::unique_ptr<RHIDescriptorSet>> m_DescriptorSets;

		std::shared_ptr<Image> m_TextureImage;
		std::shared_ptr<Sampler> m_TextureSampler;

		std::shared_ptr<Texture> m_Texture;
			
		bool m_ParamsDirty = true; // whether the parameters have been modified since the last update
	};
	using MaterialManager = ResourceManager<Material, std::string>;
}