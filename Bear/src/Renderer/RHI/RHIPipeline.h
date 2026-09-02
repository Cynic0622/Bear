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
		std::shared_ptr<RHIDescriptorSetLayout> descriptorSetLayout; // use rhi interface instead of concrete class
	};
	struct RHIDepthStencilState {
		bool depthTestEnable = true;
		bool depthWriteEnable = true;
		CompareOp depthCompareOp = CompareOp::Less;
	};

	struct RHIColorBlendAttachmentState {
		bool blendEnable = false;
		BlendFactor srcColorBlendFactor = BlendFactor::One;
		BlendFactor dstColorBlendFactor = BlendFactor::Zero;
		BlendOp colorBlendOp = BlendOp::Add;
		BlendFactor srcAlphaBlendFactor = BlendFactor::One;
		BlendFactor dstAlphaBlendFactor = BlendFactor::Zero;
		BlendOp alphaBlendOp = BlendOp::Add;
		ColorWriteMask colorWriteMask = ColorWriteMask::All;
	};


	struct RHIPipelineConfig {
		std::shared_ptr<RHIPipelineLayout> pipelineLayout;

		
		std::string vertexShaderPath;
		std::string fragmentShaderPath;
		std::string computeShaderPath;

		// ��Ⱦ״̬
		PrimitiveTopology topology = PrimitiveTopology::TriangleList;
		PolygonMode polygonMode = PolygonMode::Fill;
		CullMode cullMode = CullMode::Back;
		FrontFace frontFace = FrontFace::CounterClockwise;

		RHIColorBlendAttachmentState colorBlendAttachmentState;
		RHIDepthStencilState depthStencilState;
		uint16_t subpassIndex = 0;

		bool vertexInput = true;

		// ... δ���������� BlendState, DepthState ��
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
		virtual void UpdateBuffer(uint32_t binding, const RHIBuffer& buffer) = 0;
	};
}