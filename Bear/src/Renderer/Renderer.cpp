#include "bearpch.h"
#include "Renderer.h"
#include "RHI/RHIDevice.h"
#include "RHI/RHI.h"
#include "RHI/RHISwapchain.h"
namespace Bear {
	Renderer::Renderer(GLFWwindow* window, GraphicsAPI api)
		:m_Window(window)
	{
		Init(api);
	}
	Renderer::~Renderer()
	{
		if (m_Device) {
			m_Device->WaitIdle();
		}
	}
	void Renderer::Init(GraphicsAPI api)
	{
		RHIPlatformData platformData{};
		platformData.windowHandle = m_Window;

		m_Device = CreateDevice(api, platformData);

		RHIAttachmentDescription colorAttachment{};
		colorAttachment.format = PixelFormat::B8G8R8A8_SRGB; // 假设这是交换链最常用的格式
		colorAttachment.loadOp = AttachmentLoadOp::Clear;
		colorAttachment.storeOp = AttachmentStoreOp::Store;
		colorAttachment.initialLayout = ImageLayout::Undefined;
		colorAttachment.finalLayout = ImageLayout::PresentSrc;

		RHIAttachmentDescription depthAttachment{};
		depthAttachment.format = PixelFormat::D32_SFLOAT; // 假设我们有一个深度格式的枚举
		depthAttachment.loadOp = AttachmentLoadOp::Clear;
		depthAttachment.storeOp = AttachmentStoreOp::DontCare;
		depthAttachment.initialLayout = ImageLayout::Undefined;
		depthAttachment.finalLayout = ImageLayout::DepthStencilAttachment;
		m_RenderPass = m_Device->CreateRenderPass({ colorAttachment, depthAttachment });

		m_Swapchain = m_Device->CreateSwapchain(m_RenderPass);

		LoadResources();
	}
	void Renderer::LoadResources()
	{
	}
	void Renderer::DrawFrame() {

		RHICommandList* cmd = m_Device->BeginFrame();

		uint32_t imageIndex = m_Device->AcquireNextImage(*m_Swapchain);
		std::vector<RHIClearValue> clearValues = { /* ... */ };
		cmd->BeginRenderPass(
			m_RenderPass.get(),
			m_Swapchain->GetFramebuffer(imageIndex), // Swapchain 内部知道当前是哪个
			m_Swapchain->GetWidth(),
			m_Swapchain->GetHeight(),
			clearValues
		);
		/*for (const auto& obj : m_RenderObjects) {
			uint32_t frameIndex = m_Device->GetCurrentFrameIndex();
			obj->material->UpdateUniformBuffer(frameIndex, ubo);
			obj->material->Bind(*cmd, frameIndex);
			obj->mesh->Bind(*cmd);
			obj->mesh->Draw(*cmd);
		}*/

		cmd->EndRenderPass();
		cmd->End();

		m_Device->SubmitCommands(cmd);
		m_Device->Present(*m_Swapchain, imageIndex);
	}
}