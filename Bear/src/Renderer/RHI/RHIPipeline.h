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

		// dynamic rendering path (vkCmdBeginRendering): when enabled, the pipeline is created
		// without a VkRenderPass and declares its attachment formats instead
		bool dynamicRendering = false;
		std::vector<PixelFormat> colorFormats;
		PixelFormat depthFormat = PixelFormat::Unknown;

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
		// mipLevel = UINT32_MAX binds the full mip chain
		virtual void UpdateTexture(uint32_t binding, const RHIImage& image, const RHISampler& sampler, uint32_t mipLevel = UINT32_MAX) = 0;
		// combined image sampler with an explicit image layout (e.g. GENERAL for persistent compute resources)
		virtual void UpdateSampledImage(uint32_t binding, const RHIImage& image, const RHISampler& sampler, uint32_t mipLevel, ImageLayout layout) = 0;
		virtual void UpdateStorageImage(uint32_t binding, const RHIImage& image, uint32_t mipLevel) = 0;
		virtual void UpdateBuffer(uint32_t binding, const RHIBuffer& buffer) = 0;
	};
}