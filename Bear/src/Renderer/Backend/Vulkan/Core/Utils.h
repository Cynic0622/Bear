#pragma once
#include "Device.h"

namespace Bear {

    VkDescriptorType ToVulkanDescriptorType(Bear::DescriptorType type);

    VkShaderStageFlags ToVulkanShaderStage(Bear::ShaderStage stage);

    VkBufferUsageFlags ToVulkanBufferUsage(Bear::BufferUsage usage);

	// -------------------------- pipeline utils --------------------------
    VkPrimitiveTopology ToVulkanTopology(PrimitiveTopology topology);

    VkPolygonMode ToVulkanPolygonMode(PolygonMode mode);

    VkCullModeFlags ToVulkanCullMode(CullMode mode);

    VkFrontFace ToVulkanFrontFace(FrontFace face);

	VkCompareOp ToVulkanCompareOp(CompareOp op);

	VkBlendFactor ToVulkanBlendFactor(BlendFactor factor);

	VkBlendOp ToVulkanBlendOp(BlendOp op);

	VkColorComponentFlags ToVulkanColorWriteMask(ColorWriteMask components);

	// -------------------------- render pass utils --------------------------
    VkAttachmentLoadOp ToVulkanLoadOp(AttachmentLoadOp op);

    VkAttachmentStoreOp ToVulkanStoreOp(AttachmentStoreOp op);

    VkImageLayout ToVulkanImageLayout(ImageLayout layout);

    VkFormat ToVulkanFormat(PixelFormat format);
    PixelFormat FromVulkanFormat(VkFormat format);

    VkFormat FindDepthFormat(const Device& device);

    uint32_t FindMemoryType(VkPhysicalDevice& physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

	// -------------------------- sampler utils --------------------------
    VkFilter ToVulkanFilter(Bear::Filter filter);
    VkSamplerMipmapMode ToVulkanMipmapMode(Bear::MipmapMode mode);
    VkSamplerAddressMode ToVulkanAddressMode(Bear::SamplerAddressMode mode);
    VkBorderColor ToVulkanBorderColor(Bear::BorderColor color);

	// -------------------------- push constant range utils --------------------------
	VkPushConstantRange ToVulkanPushConstantRange(const RHIPushConstantRange& range);
}