#include "bearpch.h"
#include "Renderer.h"
#include "RHI/RHIDevice.h"
#include "RHI/RHI.h"
#include "RHI/RHISwapchain.h"
#include "Common/RenderObject.h"
#include  "Core/Types.h"
#include "Scene/SceneLayer.h"

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
	uint32_t Renderer::GetSwapchainImageCount() const
	{
		return m_Swapchain ? m_Swapchain->GetImageCount() : 0;
	}
	void Renderer::BeginFrame()
	{
		m_CurrentCommandBuffer = m_Device->BeginFrame();
		m_CurrentImageIndex = m_Device->AcquireNextImage(*m_Swapchain);
		if (m_CurrentImageIndex == UINT32_MAX) {
			return; // error acquiring image
		}
		std::vector<RHIClearValue> clearValues = { {{0.1f, 0.1f, 0.1f}, {}, false}, {{}, {}, true} };
		m_CurrentCommandBuffer->BeginRenderPass(*m_RenderPass, *m_Swapchain->GetFramebuffer(m_CurrentImageIndex), m_Swapchain->GetWidth(), m_Swapchain->GetHeight(), clearValues);
		uint32_t width, height;
		m_Swapchain->GetExtent(width, height);
		m_CurrentCommandBuffer->SetViewport(0, 0, static_cast<float>(width), static_cast<float>(height));
		m_CurrentCommandBuffer->SetScissor(0, 0, width, height);
	}

	void Renderer::Submit(const std::vector<RenderObject>& renderObjects, const SceneData& sceneData) const
	{
		for (const auto& obj : renderObjects) {
			UniformBufferObject ubo{};
			auto frameIndex = m_Device->GetCurrentFrameIndex();
			
			ubo.view = sceneData.viewMaterix;
			ubo.proj = sceneData.projectionMatrix;
			obj.material->UpdateUniformBuffer(frameIndex, ubo);
			obj.material->Bind(*m_CurrentCommandBuffer, frameIndex);
			obj.mesh->Bind(*m_CurrentCommandBuffer);
			PerObjectPushConstants pushConstants{ .model = obj.transformComponent.GetTransform() };
			m_CurrentCommandBuffer->PushConstants(*obj.material->GetPipelineLayout(), ShaderStage::Vertex, &pushConstants, sizeof(PerObjectPushConstants), 0);
			obj.mesh->Draw(*m_CurrentCommandBuffer);
		}
	}
	void Renderer::EndFrame() const
	{
		m_CurrentCommandBuffer->EndRenderPass();
		m_Device->EndFrame(*m_Swapchain, m_CurrentImageIndex);
	}
	void Renderer::Init(GraphicsAPI api)
	{
		RHIPlatformData platformData{};
		platformData.windowHandle = m_Window;

		m_Device = CreateDevice(api, platformData);

		RHIAttachmentDescription colorAttachment{};
		colorAttachment.format = PixelFormat::B8G8R8A8_SRGB;
		colorAttachment.loadOp = AttachmentLoadOp::Clear;
		colorAttachment.storeOp = AttachmentStoreOp::Store;
		colorAttachment.initialLayout = ImageLayout::Undefined;
		colorAttachment.finalLayout = ImageLayout::PresentSrc;

		RHIAttachmentDescription depthAttachment{};
		depthAttachment.format = PixelFormat::D32_SFLOAT;
		depthAttachment.loadOp = AttachmentLoadOp::Clear;
		depthAttachment.storeOp = AttachmentStoreOp::DontCare;
		depthAttachment.initialLayout = ImageLayout::Undefined;
		depthAttachment.finalLayout = ImageLayout::DepthStencilAttachment;
		m_RenderPass = m_Device->CreateRenderPass({ colorAttachment, depthAttachment });

		m_Swapchain = m_Device->CreateSwapchain(*m_RenderPass);

		m_TextureManager = std::make_unique<TextureManager>();
		m_MeshManager = std::make_unique<MeshManger>();

		LoadResources();
	}
	void Renderer::LoadResources()
	{

	}
	
	void Renderer::OnWindowResized() const
	{
		if (m_Swapchain) {
			m_Swapchain->Resize();
		}
	}
}