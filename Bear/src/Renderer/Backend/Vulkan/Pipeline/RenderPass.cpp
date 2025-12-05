#include "bearpch.h"

#include "RenderPass.h"
#include "Core/Device.h"
#include "Presentation/Swapchain.h"
#include "Core/Utils.h"
#include "RHI/RHITypes.h"

namespace Bear {
	
	RenderPass::RenderPass(const Device& device, const VkRenderPassCreateInfo& renderPassInfo)
		:m_Device(device)
	{
		/*CreateRenderPass(attachments);*/
		BEAR_CORE_ASSERT(vkCreateRenderPass(m_Device.GetDevice(), &renderPassInfo, nullptr, &m_RenderPass) == VK_SUCCESS,
			"Failed to create render pass!");
		BEAR_CORE_INFO("Vulkan RenderPass created successfully.");
	}

	RenderPass::RenderPass(const Device& device, const Swapchain& swapchain)
		:m_Device(device)
	{
		// 暂时写死交换链中的附件描述
		// std::vector<AttachmentDescription> attachments = {
		// 	{ PixelFormat::B8G8R8A8_SRGB, AttachmentLoadOp::Load, AttachmentSamples::Count1, AttachmentStoreOp::Store, AttachmentStoreOp::DontCare, AttachmentLoadOp::DontCare, ImageLayout::ColorAttachment, ImageLayout::PresentSrc },
		// 	{ PixelFormat::D32_SFLOAT, AttachmentLoadOp::Clear, AttachmentStoreOp::Store, ImageLayout::DepthStencilAttachment, ImageLayout::DepthStencilAttachment }
		// };
		// CreateRenderPass(attachments);
		BEAR_CORE_INFO("Vulkan RenderPass created successfully with swapchain.");
	}

	RenderPass::~RenderPass()
	{
		if (m_RenderPass != VK_NULL_HANDLE) {
			vkDestroyRenderPass(m_Device.GetDevice(), m_RenderPass, nullptr);
			m_RenderPass = VK_NULL_HANDLE;
#ifdef BEAR_DEBUG
			BEAR_CORE_INFO("Vulkan RenderPass destroyed successfully.");
#endif // BEAR_DEBUG
		}
	}
	void RenderPass::CreateRenderPass(const std::vector<AttachmentDescription>& attachments)
	{
		std::vector<VkAttachmentDescription> vkAttachments;
		vkAttachments.reserve(attachments.size());

		for (const auto& attachment : attachments) {
			VkAttachmentDescription vkAttachment{};
			vkAttachment.format = ToVulkanFormat(attachment.format);
			vkAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // temp
			vkAttachment.loadOp = ToVulkanLoadOp(attachment.loadOp);
			vkAttachment.storeOp = ToVulkanStoreOp(attachment.storeOp);
			vkAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			vkAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			vkAttachment.initialLayout = ToVulkanImageLayout(attachment.initialLayout);
			vkAttachment.finalLayout = ToVulkanImageLayout(attachment.finalLayout);
			vkAttachments.push_back(vkAttachment);
		}

		// 2. 定义子流程 (Subpass) 和附件引用

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0; // 附件数组中的索引
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		// 3. 定义子流程依赖 (Subpass Dependency)
		// 确保在我们可以写入颜色之前，图像已经从呈现引擎转换到适合渲染的布局
		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // 隐含的外部子流程
		dependency.dstSubpass = 0; // 我们的子流程
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		// 4. 创建 Render Pass
		//std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(vkAttachments.size());
		renderPassInfo.pAttachments = vkAttachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		BEAR_CORE_ASSERT(vkCreateRenderPass(m_Device.GetDevice(), &renderPassInfo, nullptr, &m_RenderPass) == VK_SUCCESS,
			"Failed to create render pass!");
	}
}