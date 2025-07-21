#include "bearpch.h"

#include "VulkanRenderPass.h"
#include "Core/VulkanDevice.h"
#include "Presentation/VulkanSwapchain.h"

namespace Bear {
	
	VulkanRenderPass::VulkanRenderPass(const VulkanDevice& device, const VulkanSwapchain& swapchain)
		:m_Device(device), m_Swapchain(swapchain)
	{
		CreateRenderPass();
		BEAR_CORE_INFO("Vulkan RenderPass created successfully.");
	}

	VulkanRenderPass::~VulkanRenderPass()
	{
		if (m_RenderPass != VK_NULL_HANDLE) {
			vkDestroyRenderPass(m_Device.GetHandle(), m_RenderPass, nullptr);
			m_RenderPass = VK_NULL_HANDLE;
#ifdef BEAR_DEBUG
			BEAR_CORE_INFO("Vulkan RenderPass destroyed successfully.");
#endif // BEAR_DEBUG
		}
	}
	void VulkanRenderPass::CreateRenderPass()
	{
		// 颜色附件 (来自交换链)
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = m_Swapchain.GetImageFormat();
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // 无多重采样
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // 开始时清除
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // 结束时保存
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // 渲染前布局不重要
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // 渲染后用于呈现

		// 深度附件
		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = VK_FORMAT_D32_SFLOAT;
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // 深度信息在渲染后通常不再需要
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

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
		std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		BEAR_CORE_ASSERT(vkCreateRenderPass(m_Device.GetHandle(), &renderPassInfo, nullptr, &m_RenderPass) == VK_SUCCESS,
			"Failed to create render pass!");
	}
}