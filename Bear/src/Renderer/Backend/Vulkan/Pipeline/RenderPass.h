#pragma once
#include <vulkan/vulkan.h>
#include "RHI/RHIRenderPass.h"
namespace Bear {

	class Device;
	class Swapchain;
	struct AttachmentDescription;

	class RenderPass : public RHIRenderPass {

	public:
		RenderPass(const Device& device, const VkRenderPassCreateInfo& renderPassInfo);
		RenderPass(const Device& device, const Swapchain& swapchain);
		~RenderPass();

		// ½ûÖ¹¿½±´ºÍÒÆ¶¯
		RenderPass(const RenderPass&) = delete;
		RenderPass& operator=(const RenderPass&) = delete;
		RenderPass(RenderPass&&) = delete;
		RenderPass& operator=(RenderPass&&) = delete;

		void* GetNativeHandle() const override { return m_RenderPass; }
		VkRenderPass GetHandle() const { return m_RenderPass; }

	private:
		const Device& m_Device;
		VkRenderPass m_RenderPass = VK_NULL_HANDLE;

	private:
		void CreateRenderPass(const std::vector<AttachmentDescription>& attachments);
	};
}