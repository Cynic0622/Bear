#include "bearpch.h"
#include "Utils.h"

namespace Bear {

    VkDescriptorType ToVulkanDescriptorType(Bear::DescriptorType type)
    {
        switch (type) {
        case Bear::DescriptorType::UniformBuffer:         return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case Bear::DescriptorType::CombinedImageSampler:  return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case Bear::DescriptorType::StorageBuffer:         return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        default:
            BEAR_CORE_ERROR("Unsupported DescriptorType: {}", static_cast<int>(type));
        }
    }
    VkShaderStageFlags ToVulkanShaderStage(Bear::ShaderStage stage)
    {
        VkShaderStageFlags flags = 0;
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(Bear::ShaderStage::Vertex))   flags |= VK_SHADER_STAGE_VERTEX_BIT;
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(Bear::ShaderStage::Fragment)) flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(Bear::ShaderStage::Compute))  flags |= VK_SHADER_STAGE_COMPUTE_BIT;
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(Bear::ShaderStage::Geometry))  flags |= VK_SHADER_STAGE_GEOMETRY_BIT;
        return flags;
    }
    VkBufferUsageFlags ToVulkanBufferUsage(Bear::BufferUsage usage)
    {
        VkBufferUsageFlags flags = 0;
        if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(Bear::BufferUsage::VertexBuffer))      flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(Bear::BufferUsage::IndexBuffer))       flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(Bear::BufferUsage::UniformBuffer))     flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(Bear::BufferUsage::StorageBuffer))     flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(Bear::BufferUsage::TransferSrcBuffer)) flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(Bear::BufferUsage::TransferDstBuffer)) flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        return flags;
    }
    VkPrimitiveTopology ToVulkanTopology(PrimitiveTopology topology)
    {
        switch (topology) {
        case PrimitiveTopology::TriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            // ...
        default: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        }
    }
    VkPolygonMode ToVulkanPolygonMode(PolygonMode mode)
    {
        switch (mode) {
        case PolygonMode::Fill: return VK_POLYGON_MODE_FILL;
        case PolygonMode::Line: return VK_POLYGON_MODE_LINE;
            // ...
        default: return VK_POLYGON_MODE_FILL;
        }
    }
    VkCullModeFlags ToVulkanCullMode(CullMode mode)
    {
        switch (mode) {
        case CullMode::None: return VK_CULL_MODE_NONE;
        case CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
        case CullMode::Back: return VK_CULL_MODE_BACK_BIT;
        default: return VK_CULL_MODE_BACK_BIT;
        }
    }
    VkFrontFace ToVulkanFrontFace(FrontFace face)
    {
        switch (face) {
        case FrontFace::CounterClockwise: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
        case FrontFace::Clockwise: return VK_FRONT_FACE_CLOCKWISE;
        default: return VK_FRONT_FACE_CLOCKWISE;
        }
    }
    VkAttachmentLoadOp ToVulkanLoadOp(AttachmentLoadOp op)
    {
        switch (op) {
        case AttachmentLoadOp::Load: return VK_ATTACHMENT_LOAD_OP_LOAD;
        case AttachmentLoadOp::Clear: return VK_ATTACHMENT_LOAD_OP_CLEAR;
        case AttachmentLoadOp::DontCare: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        }
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }

    VkAttachmentStoreOp Bear::ToVulkanStoreOp(AttachmentStoreOp op)
    {
        switch (op) {
        case AttachmentStoreOp::Store: return VK_ATTACHMENT_STORE_OP_STORE;
        case AttachmentStoreOp::DontCare: return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }
        return VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }

    VkImageLayout ToVulkanImageLayout(ImageLayout layout)
    {
        switch (layout) {
        case ImageLayout::Undefined: return VK_IMAGE_LAYOUT_UNDEFINED;
        case ImageLayout::ColorAttachment: return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        case ImageLayout::DepthStencilAttachment: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        case ImageLayout::PresentSrc: return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }
        return VK_IMAGE_LAYOUT_UNDEFINED;
    }

    VkFormat ToVulkanFormat(PixelFormat format)
    {
        switch (format) {
        case PixelFormat::R8G8B8A8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
        case PixelFormat::R8G8B8A8_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
        case PixelFormat::B8G8R8A8_SRGB: return VK_FORMAT_B8G8R8A8_SRGB;
        case PixelFormat::R32G32B32A32_SFLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
        case PixelFormat::D16_UNORM: return VK_FORMAT_D16_UNORM;
        case PixelFormat::D24_UNORM_S8_UINT: return VK_FORMAT_D24_UNORM_S8_UINT;
        case PixelFormat::D32_SFLOAT: return VK_FORMAT_D32_SFLOAT;
        default:
            BEAR_CORE_ERROR("Unsupported Format: {}", static_cast<int>(format));
            return VK_FORMAT_UNDEFINED;
        }
    }

    VkFormat FindDepthFormat(const Device& device)
    {
        std::vector<VkFormat> candidates = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(device.GetPhysicalDevice(), format, &props);
            if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
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
}
