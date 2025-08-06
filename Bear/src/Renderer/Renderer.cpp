#include "bearpch.h"
#include "Renderer.h"
#include "RHI/RHIDevice.h"
#include "RHI/RHI.h"
#include "RHI/RHISwapchain.h"
#include "Common/RenderObject.h"
#include  "Core/Types.h"
#include "Bear/LayerStack.h"
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
		m_PipelineLayout = m_Device->CreatePipelineLayout({});

		m_Mesh = m_MeshManager->Load("../../../models/planet/planet.obj" ,*m_Device, "../../../models/planet/planet.obj");
		m_Texture = m_TextureManager->Load("../../../models/planet/mars.png", *m_Device, "../../../models/planet/mars.png");
		// m_SquareMesh = std::make_shared<Mesh>(*m_Device, vertices, indices);
		std::vector<std::string> shaderPaths = {
			"../../../Bear/src/Bear/Shaders/tri.vert.spv",
			"../../../Bear/src/Bear/Shaders/tri.frag.spv"
		};
		m_SimpleMaterial = std::make_shared<Material>(*m_Device, *m_RenderPass, shaderPaths);
		m_SimpleMaterial->SetTexture(1, m_Texture);
		auto square1 = RenderObject::Create(m_Mesh, m_SimpleMaterial);
		square1->transform.translation = { -0.5f, 0.5f, 0.f };
		square1->transform.scale = { 0.5f, 0.5f, 0.5f };
		m_RenderObjects.push_back(std::move(square1));

	}
	void Renderer::DrawFrame(LayerStack& layerStack)
	{
		RHICommandList& cmd = m_Device->BeginFrame();

		uint32_t imageIndex = m_Device->AcquireNextImage(*m_Swapchain);
		if (imageIndex == UINT32_MAX) {
			return; // 交换链已被重建或其他错误发生
		}
		std::vector<RHIClearValue> clearValues = { {{0.1f, 0.1f, 0.1f}, {}, false}, {{}, {}, true} };
		cmd.BeginRenderPass(*m_RenderPass, *m_Swapchain->GetFramebuffer(imageIndex), m_Swapchain->GetWidth(), m_Swapchain->GetHeight(), clearValues);
		// 设置视口和裁剪区域
		uint32_t width = m_Swapchain->GetWidth();
		uint32_t height = m_Swapchain->GetHeight();
		cmd.SetViewport(0, 0, (float)width, (float)height);
		cmd.SetScissor(0, 0, width, height);
		for (const auto& obj : m_RenderObjects) {
			UniformBufferObject ubo{};
			auto frameIndex = m_Device->GetCurrentFrameIndex();
			ubo.model = obj->transform.GetTransform();
			ubo.model = glm::rotate(ubo.model, (float)glm::radians(glfwGetTime()) * 100.f, glm::vec3(0.f, 1.f, 0.f));
			ubo.view = glm::lookAt(glm::vec3(0.f, -5.f, 10.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
			ubo.proj = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);
			obj->material->UpdateUniformBuffer(frameIndex, ubo);
			obj->material->Bind(cmd, frameIndex);
			obj->mesh->Bind(cmd);
			obj->mesh->Draw(cmd);
		}
		for (auto layer : layerStack)
		{
			layer->OnRender(cmd);
		}

		cmd.EndRenderPass();
		m_Device->EndFrame(*m_Swapchain, imageIndex);
	}
	void Renderer::OnWindowResized()
	{
		if (m_Swapchain) {
			m_Swapchain->Resize();
		}
	}
}