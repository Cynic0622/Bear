#pragma once
#include "Pass.h"
#include "Common/RenderObject.h"
#include <vulkan/vulkan.h>

namespace Bear
{
	struct RenderObject;
	class UIPass : public Pass
	{
	public:
		~UIPass() override;
		void Setup(RenderContext* context) override;
		const char* GetName() const override { return "UIPass"; }
		void Execute(RHICommandList* cmd, std::vector<RenderObject> renderObjects = {});
		void Resize();
		void Cleanup() override;

	private:
		std::shared_ptr<RHIRenderPass> m_RenderPass;
		std::vector<std::shared_ptr<RHIFramebuffer>> m_Framebuffers;
		VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
	private:
		void InitGUI();
	};
}
