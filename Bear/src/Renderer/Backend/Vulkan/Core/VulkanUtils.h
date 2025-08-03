#pragma once
#include "VulkanDevice.h"

namespace Bear {

    VkDescriptorType ToVulkanDescriptorType(Bear::DescriptorType type);

    VkShaderStageFlags ToVulkanShaderStage(Bear::ShaderStage stage);

    VkBufferUsageFlags ToVulkanBufferUsage(Bear::BufferUsage usage);

    VkPrimitiveTopology ToVulkanTopology(PrimitiveTopology topology);

    VkPolygonMode ToVulkanPolygonMode(PolygonMode mode);

    VkCullModeFlags ToVulkanCullMode(CullMode mode);

    VkFrontFace ToVulkanFrontFace(FrontFace face);

    VkAttachmentLoadOp ToVulkanLoadOp(AttachmentLoadOp op);

    VkAttachmentStoreOp ToVulkanStoreOp(AttachmentStoreOp op);

    VkImageLayout ToVulkanImageLayout(ImageLayout layout);

    VkFormat ToVulkanFormat(PixelFormat format);

    VkFormat FindDepthFormat(const VulkanDevice& device);

    uint32_t FindMemoryType(VkPhysicalDevice& physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
}