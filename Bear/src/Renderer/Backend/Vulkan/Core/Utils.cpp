#include "bearpch.h"
#include "Utils.h"

namespace Bear
{
	VkDescriptorType ToVulkanDescriptorType(Bear::DescriptorType type)
	{
		switch (type)
		{
		case Bear::DescriptorType::UniformBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case Bear::DescriptorType::CombinedImageSampler: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		case Bear::DescriptorType::StorageBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		case DescriptorType::StorageImage: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		default:
			BEAR_CORE_ERROR("Unsupported DescriptorType: {}", static_cast<int>(type));
		}
	}

	VkShaderStageFlags ToVulkanShaderStage(Bear::ShaderStage stage)
	{
		VkShaderStageFlags flags = 0;
		if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(Bear::ShaderStage::Vertex)) flags |=
			VK_SHADER_STAGE_VERTEX_BIT;
		if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(Bear::ShaderStage::Fragment)) flags |=
			VK_SHADER_STAGE_FRAGMENT_BIT;
		if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(Bear::ShaderStage::Compute)) flags |=
			VK_SHADER_STAGE_COMPUTE_BIT;
		if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(Bear::ShaderStage::Geometry)) flags |=
			VK_SHADER_STAGE_GEOMETRY_BIT;
		return flags;
	}

	VkBufferUsageFlags ToVulkanBufferUsage(Bear::BufferUsage usage)
	{
		VkBufferUsageFlags flags = 0;
		if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(Bear::BufferUsage::VertexBuffer)) flags |=
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(Bear::BufferUsage::IndexBuffer)) flags |=
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(Bear::BufferUsage::StagingBuffer)) flags |=
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(Bear::BufferUsage::UniformBuffer)) flags |=
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(Bear::BufferUsage::StorageBuffer)) flags |=
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(Bear::BufferUsage::TransferSrcBuffer)) flags |=
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(Bear::BufferUsage::TransferDstBuffer)) flags |=
			VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		return flags;
	}

	VkImageUsageFlags ToVulkanImageUsage(Bear::ImageUsage usage)
	{
		VkImageUsageFlags flags = 0;
		if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(Bear::ImageUsage::ColorAttachment)) flags |=
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(Bear::ImageUsage::DepthStencilAttachment)) flags |=
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(Bear::ImageUsage::Sampled)) flags |=
			VK_IMAGE_USAGE_SAMPLED_BIT;
		if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(Bear::ImageUsage::Storage)) flags |=
			VK_IMAGE_USAGE_STORAGE_BIT;
		if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(Bear::ImageUsage::TransferSrc)) flags |=
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(Bear::ImageUsage::TransferDst)) flags |=
			VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		return flags;
	}

	VkPrimitiveTopology ToVulkanTopology(PrimitiveTopology topology)
	{
		switch (topology)
		{
		case PrimitiveTopology::TriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		// ...
		default: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		}
	}

	VkPolygonMode ToVulkanPolygonMode(PolygonMode mode)
	{
		switch (mode)
		{
		case PolygonMode::Fill: return VK_POLYGON_MODE_FILL;
		case PolygonMode::Line: return VK_POLYGON_MODE_LINE;
		// ...
		default: return VK_POLYGON_MODE_FILL;
		}
	}

	VkCullModeFlags ToVulkanCullMode(CullMode mode)
	{
		switch (mode)
		{
		case CullMode::None: return VK_CULL_MODE_NONE;
		case CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
		case CullMode::Back: return VK_CULL_MODE_BACK_BIT;
		default: return VK_CULL_MODE_BACK_BIT;
		}
	}

	VkFrontFace ToVulkanFrontFace(FrontFace face)
	{
		switch (face)
		{
		case FrontFace::CounterClockwise: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
		case FrontFace::Clockwise: return VK_FRONT_FACE_CLOCKWISE;
		default: return VK_FRONT_FACE_CLOCKWISE;
		}
	}

	VkCompareOp ToVulkanCompareOp(CompareOp op)
	{
		switch (op)
		{
		case CompareOp::Never: return VK_COMPARE_OP_NEVER;
		case CompareOp::Less: return VK_COMPARE_OP_LESS;
		case CompareOp::Equal: return VK_COMPARE_OP_EQUAL;
		case CompareOp::LessOrEqual: return VK_COMPARE_OP_LESS_OR_EQUAL;
		case CompareOp::Greater: return VK_COMPARE_OP_GREATER;
		case CompareOp::NotEqual: return VK_COMPARE_OP_NOT_EQUAL;
		case CompareOp::GreaterOrEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
		case CompareOp::Always: return VK_COMPARE_OP_ALWAYS;
		}
		return VK_COMPARE_OP_ALWAYS; // Default to always pass
	}

	VkBlendFactor ToVulkanBlendFactor(BlendFactor factor)
	{
		switch (factor)
		{
		case BlendFactor::Zero: return VK_BLEND_FACTOR_ZERO;
		case BlendFactor::One: return VK_BLEND_FACTOR_ONE;
		case BlendFactor::SrcColor: return VK_BLEND_FACTOR_SRC_COLOR;
		case BlendFactor::OneMinusSrcColor: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		case BlendFactor::DstColor: return VK_BLEND_FACTOR_DST_COLOR;
		case BlendFactor::OneMinusDstColor: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		case BlendFactor::SrcAlpha: return VK_BLEND_FACTOR_SRC_ALPHA;
		case BlendFactor::OneMinusSrcAlpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		// case BlendFactor::DstAlpha: return VK_BLEND_FACTOR_DST_ALPHA;
		// case BlendFactor::OneMinusDstAlpha: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		default:
			BEAR_CORE_WARN("Unsupported BlendFactor: {}", static_cast<int>(factor));
			return VK_BLEND_FACTOR_ZERO; // Default to zero
		}
	}

	VkBlendOp ToVulkanBlendOp(BlendOp op)
	{
		switch (op)
		{
		case BlendOp::Add: return VK_BLEND_OP_ADD;
		case BlendOp::Subtract: return VK_BLEND_OP_SUBTRACT;
		case BlendOp::ReverseSubtract: return VK_BLEND_OP_REVERSE_SUBTRACT;
		case BlendOp::Min: return VK_BLEND_OP_MIN;
		case BlendOp::Max: return VK_BLEND_OP_MAX;
		default:
			BEAR_CORE_WARN("Unsupported BlendOp: {}", static_cast<int>(op));
			return VK_BLEND_OP_ADD; // Default to add
		}
	}

	VkColorComponentFlags ToVulkanColorWriteMask(ColorWriteMask components)
	{
		VkColorComponentFlags flags = 0;
		if (static_cast<uint32_t>(components) & static_cast<uint32_t>(ColorWriteMask::R)) flags |=
			VK_COLOR_COMPONENT_R_BIT;
		if (static_cast<uint32_t>(components) & static_cast<uint32_t>(ColorWriteMask::G)) flags |=
			VK_COLOR_COMPONENT_G_BIT;
		if (static_cast<uint32_t>(components) & static_cast<uint32_t>(ColorWriteMask::B)) flags |=
			VK_COLOR_COMPONENT_B_BIT;
		if (static_cast<uint32_t>(components) & static_cast<uint32_t>(ColorWriteMask::A)) flags |=
			VK_COLOR_COMPONENT_A_BIT;
		return flags;
	}

	VkAttachmentLoadOp ToVulkanLoadOp(AttachmentLoadOp op)
	{
		switch (op)
		{
		case AttachmentLoadOp::Load: return VK_ATTACHMENT_LOAD_OP_LOAD;
		case AttachmentLoadOp::Clear: return VK_ATTACHMENT_LOAD_OP_CLEAR;
		case AttachmentLoadOp::DontCare: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		}
		return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	}

	VkAttachmentStoreOp ToVulkanStoreOp(AttachmentStoreOp op)
	{
		switch (op)
		{
		case AttachmentStoreOp::Store: return VK_ATTACHMENT_STORE_OP_STORE;
		case AttachmentStoreOp::DontCare: return VK_ATTACHMENT_STORE_OP_DONT_CARE;
		}
		return VK_ATTACHMENT_STORE_OP_DONT_CARE;
	}

	VkImageLayout ToVulkanImageLayout(ImageLayout layout)
	{
		switch (layout)
		{
		case ImageLayout::Undefined: return VK_IMAGE_LAYOUT_UNDEFINED;
		case ImageLayout::ColorAttachment: return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		case ImageLayout::DepthStencilAttachment: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		case ImageLayout::PresentSrc: return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		case ImageLayout::ShaderReadOnly: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case ImageLayout::TransferDst: return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		case ImageLayout::TransferSrc: return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		case ImageLayout::General: return VK_IMAGE_LAYOUT_GENERAL;
		}
		return VK_IMAGE_LAYOUT_UNDEFINED;
	}

	VkFormat ToVulkanFormat(PixelFormat format)
	{
		switch (format)
		{
		case PixelFormat::R8G8B8A8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
		case PixelFormat::R8G8B8A8_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
		case PixelFormat::B8G8R8A8_SRGB: return VK_FORMAT_B8G8R8A8_SRGB;
		case PixelFormat::R16G16B16A16_FLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;
		case PixelFormat::R32G32B32A32_SFLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
		case PixelFormat::D16_UNORM: return VK_FORMAT_D16_UNORM;
		case PixelFormat::D24_UNORM_S8_UINT: return VK_FORMAT_D24_UNORM_S8_UINT;
		case PixelFormat::D32_SFLOAT: return VK_FORMAT_D32_SFLOAT;
		case PixelFormat::R32_UINT: return VK_FORMAT_R32_UINT;
		default:
			BEAR_CORE_ERROR("Unsupported Format: {}", static_cast<int>(format));
			return VK_FORMAT_UNDEFINED;
		}
	}

	PixelFormat FromVulkanFormat(VkFormat format)
	{
		switch (format)
		{
		case VK_FORMAT_R8G8B8A8_UNORM: return PixelFormat::R8G8B8A8_UNORM;
		case VK_FORMAT_R8G8B8A8_SRGB: return PixelFormat::R8G8B8A8_SRGB;
		case VK_FORMAT_B8G8R8A8_SRGB: return PixelFormat::B8G8R8A8_SRGB;
		case VK_FORMAT_R32G32B32A32_SFLOAT: return PixelFormat::R32G32B32A32_SFLOAT;
		case VK_FORMAT_D16_UNORM: return PixelFormat::D16_UNORM;
		case VK_FORMAT_D24_UNORM_S8_UINT: return PixelFormat::D24_UNORM_S8_UINT;
		case VK_FORMAT_D32_SFLOAT: return PixelFormat::D32_SFLOAT;
		default:
			BEAR_CORE_ERROR("Unsupported Format: {}", static_cast<int>(format));
			return PixelFormat::R8G8B8A8_UNORM;
		}
	}

	VkAttachmentDescription ToVulkanAttachmentDescription(const AttachmentDescription& desc)
	{
		VkAttachmentDescription attachment = {};
		attachment.format = ToVulkanFormat(desc.format);
		attachment.samples = static_cast<VkSampleCountFlagBits>(desc.samples);
		attachment.loadOp = ToVulkanLoadOp(desc.loadOp);
		attachment.storeOp = ToVulkanStoreOp(desc.storeOp);
		attachment.stencilLoadOp = ToVulkanLoadOp(desc.stencilLoadOp);
		attachment.stencilStoreOp = ToVulkanStoreOp(desc.storeOp);
		attachment.initialLayout = ToVulkanImageLayout(desc.initialLayout);
		attachment.finalLayout = ToVulkanImageLayout(desc.finalLayout);
		return attachment;
	}

	VkPipelineStageFlags ToVulkanPipelineStage(PipelineStage flags)
	{
		VkPipelineStageFlags vulkanFlags = 0;
		if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStage::VertexShader)) vulkanFlags |=
			VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStage::FragmentShader)) vulkanFlags |=
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStage::ComputeShader)) vulkanFlags |=
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStage::Transfer)) vulkanFlags |=
			VK_PIPELINE_STAGE_TRANSFER_BIT;
		if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStage::BottomOfPipe)) vulkanFlags |=
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStage::TopOfPipe)) vulkanFlags |=
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStage::AllGraphics)) vulkanFlags |=
			VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
		if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStage::AllCommands)) vulkanFlags |=
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStage::Host)) vulkanFlags |=
			VK_PIPELINE_STAGE_HOST_BIT;
		if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStage::DrawIndirect)) vulkanFlags |=
			VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
		if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStage::EarlyFragmentTests)) vulkanFlags |=
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStage::LateFragmentTests)) vulkanFlags |=
			VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStage::ColorAttachmentOutput)) vulkanFlags |=
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		return vulkanFlags;
	}

	VkAccessFlags ToVulkanAccessFlags(AccessFlags flags)
	{
		VkAccessFlags vulkanFlags = 0;
		if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::VertexAttributeRead)) vulkanFlags |=
			VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::IndexRead)) vulkanFlags |=
			VK_ACCESS_INDEX_READ_BIT;
		if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::UniformRead)) vulkanFlags |=
			VK_ACCESS_UNIFORM_READ_BIT;
		if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::ShaderRead)) vulkanFlags |=
			VK_ACCESS_SHADER_READ_BIT;
		if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::ShaderWrite)) vulkanFlags |=
			VK_ACCESS_SHADER_WRITE_BIT;
		if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::TransferRead)) vulkanFlags |=
			VK_ACCESS_TRANSFER_READ_BIT;
		if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::TransferWrite)) vulkanFlags |=
			VK_ACCESS_TRANSFER_WRITE_BIT;
		return vulkanFlags;
	}

	VkFormat FindDepthFormat(const Device& device)
	{
		std::vector<VkFormat> candidates = {
			VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT
		};
		for (VkFormat format : candidates)
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(device.GetPhysicalDevice(), format, &props);
			if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
			{
				return format;
			}
		}
		BEAR_CORE_ERROR("Failed to find a suitable depth format.");
	}

	uint32_t FindMemoryType(VkPhysicalDevice& physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}
		BEAR_CORE_ERROR("Failed to find suitable memory type!");
	}

	// ------------------sampler-------------------
	VkFilter ToVulkanFilter(Bear::Filter filter)
	{
		switch (filter)
		{
		case Bear::Filter::Nearest: return VK_FILTER_NEAREST;
		case Bear::Filter::Linear: return VK_FILTER_LINEAR;
		default: return VK_FILTER_LINEAR;
		}
	}

	VkSamplerMipmapMode ToVulkanMipmapMode(Bear::MipmapMode mode)
	{
		switch (mode)
		{
		case Bear::MipmapMode::Nearest: return VK_SAMPLER_MIPMAP_MODE_NEAREST;
		case Bear::MipmapMode::Linear: return VK_SAMPLER_MIPMAP_MODE_LINEAR;
		default: return VK_SAMPLER_MIPMAP_MODE_LINEAR;
		}
	}

	VkSamplerAddressMode ToVulkanAddressMode(Bear::SamplerAddressMode mode)
	{
		switch (mode)
		{
		case Bear::SamplerAddressMode::Repeat: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case Bear::SamplerAddressMode::MirroredRepeat: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		case Bear::SamplerAddressMode::ClampToEdge: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case Bear::SamplerAddressMode::ClampToBorder: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		default: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		}
	}

	VkBorderColor ToVulkanBorderColor(Bear::BorderColor color)
	{
		switch (color)
		{
		case Bear::BorderColor::FloatTransparentBlack: return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		case Bear::BorderColor::IntTransparentBlack: return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
		case Bear::BorderColor::FloatOpaqueBlack: return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		case Bear::BorderColor::IntOpaqueBlack: return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		case Bear::BorderColor::FloatOpaqueWhite: return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		case Bear::BorderColor::IntOpaqueWhite: return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
		default: return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		}
	}

	VkPushConstantRange ToVulkanPushConstantRange(const RHIPushConstantRange& range)
	{
		VkPushConstantRange pushConstantRange = {};
		pushConstantRange.stageFlags = ToVulkanShaderStage(range.stageFlags);
		pushConstantRange.offset = range.offset;
		pushConstantRange.size = range.size;
		return pushConstantRange;
	}
}
