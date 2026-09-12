#pragma once
#include "Pass.h"
#include <vulkan/vulkan.h>

namespace Bear
{
	class RHIImage;
	class UIPass : public Pass
	{
	public:
		~UIPass() override;
		void Setup(RenderContext* context) override;
		const char* GetName() const override { return "UIPass"; }
		// color is the dynamic-rendering target; the frame graph transitions it
		void Execute(RHICommandList* cmd, RHIImage* color);
		void Cleanup() override;

	private:
		VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
	private:
		void InitGUI();
	};
}
