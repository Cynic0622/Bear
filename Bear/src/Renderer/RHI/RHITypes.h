#pragma once
#include <cstdint>

namespace Bear {

    // 定义一个颜色结构体
    struct ClearColor {
        float r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;
    };

    // 定义一个深度/模板结构体
    struct ClearDepthStencil {
        float depth = 1.0f;
        uint32_t stencil = 0;
    };

    // RHI 级别的清除值结构体
    struct RHIClearValue {
        ClearColor color;
        ClearDepthStencil depthStencil;
        // 标志来区分这个清除值是用于颜色还是深度
        bool isDepth = false;
    };

    enum class PrimitiveTopology {
        TriangleList,
        TriangleStrip,
        LineList,
        // ...
    };

    enum class PolygonMode {
        Fill,
        Line,
        Point,
    };

    enum class CullMode {
        None,
        Front,
        Back,
    };

    enum class FrontFace {
        CounterClockwise,
        Clockwise,
    };

    enum class BufferUsage : uint32_t {
        None = 0,
        VertexBuffer = 1 << 0,
        IndexBuffer = 1 << 1,
        UniformBuffer = 1 << 2,
        StagingBuffer = 1 << 3,
		StorageBuffer = 1 << 4,
		TransferSrcBuffer = 1 << 5,
        TransferDstBuffer = 1 << 6
    };

    enum class DescriptorType {
        UniformBuffer,
        CombinedImageSampler,
        StorageBuffer,
    };

    enum class ShaderStage : uint32_t {
        None = 0,
        Vertex = 1 << 0,
        Fragment = 1 << 1,
        Compute = 1 << 2,
        Geometry = 1 << 3,

        AllGraphics = Vertex | Fragment | Geometry,
        All = 0xFFFFFFFF
    };
    // 组合
    inline ShaderStage operator|(ShaderStage a, ShaderStage b) {
        return static_cast<ShaderStage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline BufferUsage operator|(BufferUsage a, BufferUsage b) {
        return static_cast<BufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    struct RHIDescriptorSetLayoutBinding {
        uint32_t binding = 0;
        DescriptorType descriptorType = DescriptorType::UniformBuffer;
        uint32_t descriptorCount = 1;
        ShaderStage stageFlags = ShaderStage::None;
    };

    // 描述附件（如颜色缓冲、深度缓冲）的加载/存储操作
    enum class AttachmentLoadOp {
        Load,
        Clear,
        DontCare
    };

    enum class AttachmentStoreOp {
        Store,
        DontCare
    };

    // 描述图像的布局状态
    enum class ImageLayout {
        Undefined,
        ColorAttachment,
        TransferDst,
        ShaderReadOnly,
        DepthStencilAttachment,
        PresentSrc // 用于呈现到屏幕
    };

    enum class PixelFormat {
        Unknown,
        R8G8B8A8_UNORM,
        R8G8B8A8_SRGB,
        B8G8R8A8_SRGB,
        R32G32B32A32_SFLOAT,
        R16G16B16A16_FLOAT,
        D24_UNORM_S8_UINT,
        D32_SFLOAT_S8_UINT, // 深度和模板格式
        D16_UNORM,
        D32_SFLOAT
        // 其他格式...
	};

    struct RHIAttachmentDescription {
        PixelFormat format = PixelFormat::Unknown;
        AttachmentLoadOp loadOp = AttachmentLoadOp::Clear;
        AttachmentStoreOp storeOp = AttachmentStoreOp::Store;
        ImageLayout initialLayout = ImageLayout::Undefined;
        ImageLayout finalLayout = ImageLayout::ColorAttachment;
    };

    enum class GraphicsAPI {
        OpenGL,
        Vulkan,
        DirectX12,
        Metal,
        Null // 用于测试或不支持的情况
	};

    struct RHIPlatformData {
		void* windowHandle = nullptr;
    };

    struct RHITextureConfig {
		uint32_t width = 0;
		uint32_t height = 0;
		PixelFormat format = PixelFormat::Unknown;
    };

	// --------------------------- sampler ---------------------------
    enum class Filter {
        Nearest,
        Linear,
    };

    // Mipmap 模式
    enum class MipmapMode {
        Nearest,
        Linear,
    };

    // 寻址模式 (U, V, W 坐标)
    enum class SamplerAddressMode {
        Repeat,
        MirroredRepeat,
        ClampToEdge,
        ClampToBorder,
    };

	// border color
    enum class BorderColor {
        FloatTransparentBlack,
        IntTransparentBlack,
        FloatOpaqueBlack,
        IntOpaqueBlack,
        FloatOpaqueWhite,
        IntOpaqueWhite,
    };

    struct RHISamplerConfig {
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
        static RHISamplerConfig GetDefault() {
            return RHISamplerConfig{};
        }
    };

    struct RHIPushConstantRange
    {
        ShaderStage stageFlags;
		uint32_t offset;
		uint32_t size;
    };
}