#pragma once
#include "IRenderPass.h"
#include <vulkan/vulkan.h>

namespace Bear
{
	class UIPass : public IRenderPass
	{
	public:
		~UIPass() override;
		void Setup(RenderContext* context) override;
		void Execute(RHICommandList* cmd) override;
		void Resize() override;
		void Cleanup() override;

	private:
		RenderContext* m_Context = nullptr;
		std::shared_ptr<RHIRenderPass> m_RenderPass;
		std::vector<std::shared_ptr<RHIFramebuffer>> m_Framebuffers;
		VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
	private:
		void InitGUI();
	};
}
