#pragma once
#include <cstdint>

namespace Bear
{
	class RHIDevice;
	class RHISwapchain;
	class RHIDescriptorSetLayout;
	class RHIDescriptorSet;

	// render context
	struct RenderContext
	{
		RHIDevice* device = nullptr;
		RHISwapchain* swapchain = nullptr;
		uint8_t MAX_FRAMES_IN_FLIGHT;
		GLFWwindow* window = nullptr;
		RHIDescriptorSetLayout* baseDataDescriptorSetLayout = nullptr; // global descriptor set layout for all pass.
		std::vector<RHIDescriptorSet*> baseDataDescriptorSet; // global descriptor set for all pass.
		RHIDescriptorSetLayout* sceneDataDescriptorSetLayout = nullptr;
		std::vector<RHIDescriptorSet*> sceneDataDescriptorSet;
		bool useTextureCompression = true;
	};

	struct ClearVaule
	{
		float r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;
	};

	
	struct ClearDepthStencil
	{
		float depth = 1.0f;
		uint32_t stencil = 0;
	};

	struct RHIClearValue
	{
		ClearVaule color;
		ClearDepthStencil depthStencil;
		bool isDepth = false;
	};

	struct ClearColor
	{
		uint32_t uint32[4];
		int32_t int32[4];
		float float32[4];
	};
	

	enum class PrimitiveTopology
	{
		TriangleList,
		TriangleStrip,
		LineList,
		// ...
	};

	enum class PolygonMode
	{
		Fill,
		Line,
		Point,
	};

	enum class CullMode
	{
		None,
		Front,
		Back,
	};

	enum class FrontFace
	{
		CounterClockwise,
		Clockwise,
	};

	enum class BufferUsage : uint32_t
	{
		None = 0,
		VertexBuffer = 1 << 0,
		IndexBuffer = 1 << 1,
		UniformBuffer = 1 << 2,
		StagingBuffer = 1 << 3,
		StorageBuffer = 1 << 4,
		TransferSrcBuffer = 1 << 5,
		TransferDstBuffer = 1 << 6
	};

	enum class ImageUsage : uint32_t
	{
		None = 0,
		ColorAttachment = 1 << 0,
		DepthStencilAttachment = 1 << 1,
		Sampled = 1 << 2,
		TransferSrc = 1 << 3,
		TransferDst = 1 << 4,
		Storage = 1 << 5
	};
	inline ImageUsage operator|(ImageUsage a, ImageUsage b)
	{
		return static_cast<ImageUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	enum class DescriptorType
	{
		UniformBuffer,
		CombinedImageSampler,
		StorageBuffer,
		StorageImage,
	};

	enum class ShaderStage : uint32_t
	{
		None = 0,
		Vertex = 1 << 0,
		Fragment = 1 << 1,
		Compute = 1 << 2,
		Geometry = 1 << 3,

		AllGraphics = Vertex | Fragment | Geometry,
		All = 0xFFFFFFFF
	};

	// ���
	inline ShaderStage operator|(ShaderStage a, ShaderStage b)
	{
		return static_cast<ShaderStage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	inline BufferUsage operator|(BufferUsage a, BufferUsage b)
	{
		return static_cast<BufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	struct RHIDescriptorSetLayoutBinding
	{
		uint32_t binding = 0;
		DescriptorType descriptorType = DescriptorType::UniformBuffer;
		uint32_t descriptorCount = 1;
		ShaderStage stageFlags = ShaderStage::None;
	};

	// ��������������ɫ���塢��Ȼ��壩�ļ���/�洢����
	enum class AttachmentLoadOp
	{
		Load,
		Clear,
		DontCare
	};

	enum class AttachmentStoreOp
	{
		Store,
		DontCare
	};

	// ����ͼ��Ĳ���״̬
	enum class ImageLayout
	{
		Undefined,
		ColorAttachment,
		TransferDst,
		TransferSrc,
		ShaderReadOnly,
		DepthStencilAttachment,
		PresentSrc,
		General
	};

	enum class PixelFormat
	{
		Unknown,
		R8_SRGB,
		R32_UINT,
		R8G8_SRGB,
		R8G8B8_SRGB,
		R8G8B8A8_UNORM,
		R8G8B8A8_SRGB,
		B8G8R8A8_SRGB,
		R32G32B32A32_SFLOAT,
		R16G16B16A16_FLOAT,
		D24_UNORM_S8_UINT,
		D32_SFLOAT_S8_UINT,
		D16_UNORM,
		D32_SFLOAT
	};

    enum class AttachmentSamples
	{
		Count1 = 1,
		Count2 = 2,
		Count4 = 4,
		Count8 = 8,
		Count16 = 16
	};

	struct AttachmentDescription
	{
		PixelFormat format = PixelFormat::Unknown;
		AttachmentLoadOp loadOp = AttachmentLoadOp::Clear;
        AttachmentSamples samples = AttachmentSamples::Count1;
		AttachmentStoreOp storeOp = AttachmentStoreOp::Store;
        AttachmentStoreOp stencilStoreOp = AttachmentStoreOp::DontCare;
        AttachmentLoadOp stencilLoadOp = AttachmentLoadOp::DontCare;
		ImageLayout initialLayout = ImageLayout::Undefined;
		ImageLayout finalLayout = ImageLayout::ColorAttachment;
	};

	// capacity limits (small fixed-size arrays to avoid heap allocs)
	constexpr uint8_t RHI_MAX_ATTACHMENTS = 16;
	constexpr uint8_t RHI_MAX_SUBPASSES = 8;
	constexpr uint8_t RHI_MAX_DEPENDENCIES = 16;
	constexpr uint8_t RHI_MAX_ATTACH_PER_SUBPASS = 8;
	constexpr uint8_t RHI_MAX_PRESERVE_ATTACH_PER_SP = 8;

	// per-attachment reference within a subpass
	struct AttachmentReference
	{
		uint32_t attachment = UINT32_MAX; // index into RenderPassDescription::attachments[]
		ImageLayout layout = ImageLayout::Undefined;
	};

	// pipeline stage and access masks (bit flags)
	enum class PipelineStage : uint32_t
	{
		None = 0,
		TopOfPipe = 1 << 0,
		DrawIndirect = 1 << 1,
		VertexInput = 1 << 2,
		VertexShader = 1 << 3,
		FragmentShader = 1 << 4,
		EarlyFragmentTests = 1 << 5,
		LateFragmentTests = 1 << 6,
		ColorAttachmentOutput = 1 << 7,
		ComputeShader = 1 << 8,
		Transfer = 1 << 9,
		BottomOfPipe = 1 << 10,
		Host = 1 << 11,
		AllGraphics = 1 << 12,
		AllCommands = 1 << 13
	};

	inline PipelineStage operator|(PipelineStage a, PipelineStage b)
	{
		return static_cast<PipelineStage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	enum class AccessFlags : uint32_t
	{
		None = 0,
		IndirectCommandRead = 1 << 0,
		IndexRead = 1 << 1,
		VertexAttributeRead = 1 << 2,
		UniformRead = 1 << 3,
		InputAttachmentRead = 1 << 4,
		ShaderRead = 1 << 5,
		ShaderWrite = 1 << 6,
		ColorAttachmentRead = 1 << 7,
		ColorAttachmentWrite = 1 << 8,
		DepthStencilAttachmentRead = 1 << 9,
		DepthStencilAttachmentWrite = 1 << 10,
		TransferRead = 1 << 11,
		TransferWrite = 1 << 12,
		HostRead = 1 << 13,
		HostWrite = 1 << 14,
		MemoryRead = 1 << 15,
		MemoryWrite = 1 << 16
	};

	inline AccessFlags operator|(AccessFlags a, AccessFlags b)
	{
		return static_cast<AccessFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	// subpass description (attachments lists)
	struct SubpassDescription
	{
		// input attachments (read-only in fragment shader)
		uint8_t inputAttachmentCount = 0;
		AttachmentReference inputAttachments[RHI_MAX_ATTACH_PER_SUBPASS];

		// color attachments (render targets)
		uint8_t colorAttachmentCount = 0;
		AttachmentReference colorAttachments[RHI_MAX_ATTACH_PER_SUBPASS];

		// resolve attachments (MSAA)
		uint8_t resolveAttachmentCount = 0;
		AttachmentReference resolveAttachments[RHI_MAX_ATTACH_PER_SUBPASS];

		// optional depth/stencil
		bool hasDepthStencil = false;
		AttachmentReference depthStencilAttachment{};

		// preserve attachments (indices into attachments[])
		uint8_t preserveAttachmentCount = 0;
		uint32_t preserveAttachments[RHI_MAX_PRESERVE_ATTACH_PER_SP]{};
	};

	struct SubpassDependency
	{
		// use UINT32_MAX to denote VK_SUBPASS_EXTERNAL
		uint32_t srcSubpass = UINT32_MAX;
		uint32_t dstSubpass = UINT32_MAX;

		PipelineStage srcStageMask = PipelineStage::TopOfPipe;
		PipelineStage dstStageMask = PipelineStage::BottomOfPipe;

		AccessFlags srcAccessMask = AccessFlags::None;
		AccessFlags dstAccessMask = AccessFlags::None;

		bool byRegion = true; // framebuffer-local dependency
	};

	struct RenderPassDescription
	{
		uint8_t attachmentCount = 0;
		AttachmentDescription attachments[RHI_MAX_ATTACHMENTS];

		uint8_t subpassCount = 0;
		SubpassDescription subpasses[RHI_MAX_SUBPASSES];

		uint8_t dependencyCount = 0;
		SubpassDependency dependencies[RHI_MAX_DEPENDENCIES];
	};

	enum class GraphicsAPI
	{
		OpenGL,
		Vulkan,
		DirectX12,
		Metal,
		Null // ���ڲ��Ի�֧�ֵ����
	};

	struct RHIPlatformData
	{
		void* windowHandle = nullptr;
	};

	struct RHITextureConfig
	{
		uint32_t width = 0;
		uint32_t height = 0;
		PixelFormat format = PixelFormat::Unknown;
		ImageUsage usage = ImageUsage::None;
		uint32_t mipLevels = 1;

	};

	// --------------------------- sampler ---------------------------
	enum class Filter
	{
		Nearest,
		Linear,
	};

	// Mipmap ģʽ
	enum class MipmapMode
	{
		Nearest,
		Linear,
		None, // no mipmaps.
	};

	// Ѱַģʽ (U, V, W ����)
	enum class SamplerAddressMode
	{
		Repeat,
		MirroredRepeat,
		ClampToEdge,
		ClampToBorder,
	};

	// border color
	enum class BorderColor
	{
		FloatTransparentBlack,
		IntTransparentBlack,
		FloatOpaqueBlack,
		IntOpaqueBlack,
		FloatOpaqueWhite,
		IntOpaqueWhite,
	};

	struct RHISamplerConfig
	{
		Filter magFilter = Filter::Linear;
		Filter minFilter = Filter::Linear;
		MipmapMode mipmapMode = MipmapMode::Linear;
		SamplerAddressMode addressModeU = SamplerAddressMode::Repeat;
		SamplerAddressMode addressModeV = SamplerAddressMode::Repeat;
		SamplerAddressMode addressModeW = SamplerAddressMode::Repeat;

		bool anisotropyEnable = true;
		float maxAnisotropy = 16.0f; // default value, can be adjusted

		// don't support comparison sampler for now
		bool compareEnable = false;

		BorderColor borderColor = BorderColor::IntOpaqueBlack;

		// default config
		static RHISamplerConfig GetDefault()
		{
			return RHISamplerConfig{};
		}
	};

	struct RHIPushConstantRange
	{
		ShaderStage stageFlags;
		uint32_t size;
		uint32_t offset;
	};

	// input rate
	enum class VertexInputRate
	{
		Vertex,
		Instance
	};

	// blend factor
	enum class BlendFactor
	{
		Zero,
		One,
		SrcColor,
		OneMinusSrcColor,
		DstColor,
		OneMinusDstColor,
		SrcAlpha,
		OneMinusSrcAlpha,
	};

	// depth/stencil compare operation
	enum class CompareOp
	{
		Never,
		Less,
		Equal,
		LessOrEqual,
		Greater,
		NotEqual,
		GreaterOrEqual,
		Always
	};

	// blend operation
	enum class BlendOp
	{
		Add,
		Subtract,
		ReverseSubtract,
		Min,
		Max,
	};

	// attachment blend state
	enum class ColorWriteMask : uint8_t
	{
		None = 0,
		R = 1 << 0,
		G = 1 << 1,
		B = 1 << 2,
		A = 1 << 3,
		All = R | G | B | A
	};

	inline ColorWriteMask operator|(ColorWriteMask a, ColorWriteMask b)
	{
		return static_cast<ColorWriteMask>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
	}

	struct MemoryBarrier
	{
		PipelineStage srcStageMask = PipelineStage::TopOfPipe;
		PipelineStage dstStageMask = PipelineStage::BottomOfPipe;

		AccessFlags srcAccessMask = AccessFlags::None;
		AccessFlags dstAccessMask = AccessFlags::None;

		bool byRegion = true; // framebuffer-local dependency
	};
}
