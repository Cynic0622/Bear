#include "bearpch.h"
#include "Renderer.h"
#include "CommandBuffer.h"
#include "DescriptorSet.h"
#include "Shader.h"
#include "UIPass.h"
#include "PbrPass.h"
#include "OitPass.h"
#include "RHI/RHIDevice.h"
#include "RHI/RHI.h"
#include "RHI/RHISwapchain.h"
#include "Common/RenderObject.h"
#include "Scene/SceneLayer.h"
#include "Core/ResourceManager.h"
#include "Scene/Scene.h"
#include "SkyboxPass.h"

namespace Bear {
	Renderer::Renderer(GLFWwindow* window, GraphicsAPI api)
		:m_Window(window)
	{
		m_RenderContext = new RenderContext();
		Init(api);
	}
	Renderer::~Renderer()
	{
		if (m_Device) {
			m_Device->WaitIdle();
		}
		m_UIPass.reset();
		m_PbrPass.reset();
		m_OitPass.reset();
		m_RenderPass.reset();
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

		ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
	}

	void Renderer::Submit(const std::vector<RenderObject>& renderObjects, const SceneData& sceneData, const BaseData& baseData)
	{
		auto frameIndex = m_Device->GetCurrentFrameIndex();
		m_BaseDataUniformBuffer[frameIndex]->UploadData(&baseData, sizeof(baseData)); // set 0.
		m_SceneDataUniformBuffer[frameIndex]->UploadData(&sceneData, sizeof(sceneData));
		m_SkyboxPass->Execute(m_CurrentCommandBuffer, renderObjects);
		m_PbrPass->Execute(m_CurrentCommandBuffer, renderObjects);
		if (OitEnabled)
		{
			if (!m_OitPass)
			{
				m_OitPass = std::make_unique<OitPass>();
				m_OitPass->Setup(m_RenderContext);
			}
			m_OitPass->Execute(m_CurrentCommandBuffer, renderObjects);
		}
	}
	
	void Renderer::EndFrame() const
	{
		ImGui::Render();
		m_UIPass->Execute(m_CurrentCommandBuffer);
		m_Device->EndFrame(*m_Swapchain, m_CurrentImageIndex);
	}
	void Renderer::Init(GraphicsAPI api)
	{
		RHIPlatformData platformData{};
		platformData.windowHandle = m_Window;

		m_Device = CreateDevice(api, platformData);

		AttachmentDescription colorAttachment{};
		colorAttachment.format = PixelFormat::B8G8R8A8_SRGB;
		colorAttachment.loadOp = AttachmentLoadOp::Clear;
		colorAttachment.storeOp = AttachmentStoreOp::Store;
		colorAttachment.initialLayout = ImageLayout::Undefined;
		colorAttachment.finalLayout = ImageLayout::ColorAttachment;

		AttachmentDescription depthAttachment{};
		depthAttachment.format = PixelFormat::D32_SFLOAT;
		depthAttachment.loadOp = AttachmentLoadOp::Clear;
		depthAttachment.storeOp = AttachmentStoreOp::Store;
		depthAttachment.initialLayout = ImageLayout::Undefined;
		depthAttachment.finalLayout = ImageLayout::DepthStencilAttachment;
		m_RenderPass = m_Device->CreateRenderPass({ colorAttachment, depthAttachment });
		
		m_Swapchain = m_Device->CreateSwapchain(*m_RenderPass);

		m_BaseDataUniformBuffer.resize(MAX_FRAMES_IN_FLIGHT);
		m_BaseDataDescriptorSet.resize(MAX_FRAMES_IN_FLIGHT);
		m_SceneDataUniformBuffer.resize(MAX_FRAMES_IN_FLIGHT);
		m_SceneDataDescriptorSet.resize(MAX_FRAMES_IN_FLIGHT);

		m_BaseDataDescriptorSetLayout = m_Device->CreateDescriptorSetLayout({
			{0, DescriptorType::UniformBuffer, 1, ShaderStage::Vertex | ShaderStage::Fragment}
			});
		m_SceneDataDescriptorSetLayout = m_Device->CreateDescriptorSetLayout({
			{0, DescriptorType::UniformBuffer, 1, ShaderStage::Vertex | ShaderStage::Fragment},
			{1, DescriptorType::CombinedImageSampler, 1, ShaderStage::Fragment }
			});

		std::string filePath = "assets/skybox/kloppenheim_06_puresky_4k.hdr";
		m_IBLTexture = std::make_shared<Texture>(*m_Device, filePath);
		m_IBLDescriptorSet = m_Device->CreateDescriptorSet(m_SceneDataDescriptorSetLayout);
		m_IBLDescriptorSet->UpdateTexture(1, m_IBLTexture->GetImage(), m_IBLTexture->GetSampler());

		for (uint8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			m_BaseDataDescriptorSet[i] = m_Device->CreateDescriptorSet(m_BaseDataDescriptorSetLayout);
			m_BaseDataUniformBuffer[i] = m_Device->CreateBuffer(sizeof(BaseData), BufferUsage::UniformBuffer, true);
			m_BaseDataDescriptorSet[i]->UpdateBuffer(0, *m_BaseDataUniformBuffer[i]);
			m_SceneDataDescriptorSet[i] = m_Device->CreateDescriptorSet(m_SceneDataDescriptorSetLayout);
			m_SceneDataUniformBuffer[i] = m_Device->CreateBuffer(sizeof(SceneData), BufferUsage::UniformBuffer, true);
			m_SceneDataDescriptorSet[i]->UpdateBuffer(0, *m_SceneDataUniformBuffer[i]);
			m_SceneDataDescriptorSet[i]->UpdateTexture(1, m_IBLTexture->GetImage(), m_IBLTexture->GetSampler());
			m_RenderContext->baseDataDescriptorSet.push_back(m_BaseDataDescriptorSet[i].get());
			m_RenderContext->sceneDataDescriptorSet.push_back(m_SceneDataDescriptorSet[i].get());
		}

		m_RenderContext->device = m_Device.get();
		m_RenderContext->swapchain = m_Swapchain.get();
		m_RenderContext->window = m_Window;
		m_RenderContext->MAX_FRAMES_IN_FLIGHT = MAX_FRAMES_IN_FLIGHT;
		m_RenderContext->baseDataDescriptorSetLayout = m_BaseDataDescriptorSetLayout.get();
		m_RenderContext->sceneDataDescriptorSetLayout = m_SceneDataDescriptorSetLayout.get();
		m_RenderContext->useTextureCompression = false;

		m_Resource = std::make_unique<ResourceManager>(m_RenderContext);

		m_UIPass = std::make_unique<UIPass>();
		m_UIPass->Setup(m_RenderContext);
		m_SkyboxPass = std::make_unique<SkyboxPass>();
		m_SkyboxPass->Setup(m_RenderContext);
		m_PbrPass = std::make_unique<PbrPass>();
		m_PbrPass->Setup(m_RenderContext);
	}
	
	
	bool Renderer::OnWindowResize() const
	{
		if (m_Swapchain) {
			m_Swapchain->Resize();
		}
		if (m_UIPass)
		{
			m_UIPass->Resize();
		}
		if (m_PbrPass)
		{
			m_PbrPass->Resize();
		}
		if (m_OitPass)
		{
			m_OitPass->Resize();
		}
		if (m_SkyboxPass)
		{
			m_SkyboxPass->Resize();
		}
		return false; // Returning false to propagate the event further
	}
	bool Renderer::OnKeyPress()
	{
		if (Input::IsKeyPressed(Key::X))
		{
			OitEnabled = !OitEnabled;
			BEAR_CORE_INFO("OIT: {}", OitEnabled ? "ON" : "OFF");
			return true;
		}
		return false;
	}
	void Renderer::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e) { return this->OnWindowResize(); });
		dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& e) { return this->OnKeyPress(); });
	}
}