#pragma once
#include <memory>
#include "RHITypes.h"
namespace Bear {
	class RHIImage;
	class RHISampler;

	class RHIBuffer;
	class RHIDescriptorSetLayout {
	public:
		virtual ~RHIDescriptorSetLayout() = default;
	};

	class RHIPipelineLayout {
	public:
		virtual ~RHIPipelineLayout() = default;
	};

	struct RHIPipelineConfigInfo {
		std::shared_ptr<RHIDescriptorSetLayout> descriptorSetLayout; // 使用 RHI 接口
	};
	// 新增：API 无关的管线配置结构体
	struct RHIPipelineConfig {
		std::shared_ptr<RHIPipelineLayout> pipelineLayout;

		// 我们需要一种方法引用着色器，暂时用路径
		std::string vertexShaderPath;
		std::string fragmentShaderPath;

		// 渲染状态
		PrimitiveTopology topology = PrimitiveTopology::TriangleList;
		PolygonMode polygonMode = PolygonMode::Fill;
		CullMode cullMode = CullMode::Back;
		FrontFace frontFace = FrontFace::CounterClockwise;

		// ... 未来可以添加 BlendState, DepthState 等
	};

	class RHIPipeline {
	public:
		virtual ~RHIPipeline() = default;
	};

	class RHIDescriptorSet {
	public:
		virtual ~RHIDescriptorSet() = default;

		virtual void UpdateDescriptorSet(uint32_t binding, const RHIBuffer& buffer) = 0;
		virtual void UpdateTexture(uint32_t binding, const RHIImage& image, const RHISampler& sampler) = 0;
	};
}