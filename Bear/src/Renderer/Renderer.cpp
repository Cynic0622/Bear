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
#include  "Core/Types.h"
#include "Scene/SceneLayer.h"
#include "Core/Resource.h"
#include "Scene/Scene.h"
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

	void Renderer::Submit(const std::vector<RenderObject>& renderObjects, const SceneData& sceneData)
	{
		auto frameIndex = m_Device->GetCurrentFrameIndex();
		m_GlobalUniformBuffer[frameIndex]->UploadData(&sceneData, sizeof(sceneData)); // set 0.
		m_PbrPass->Execute(m_CurrentCommandBuffer, renderObjects);
		if (OitEnabled)
		{
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
		// m_UIRenderPass = m_Device->CreateUIRenderPass();
		
		m_Swapchain = m_Device->CreateSwapchain(*m_RenderPass);

		m_TextureManager = std::make_unique<TextureManager>();
		m_MeshManager = std::make_unique<MeshManager>();

		m_GlobalUniformBuffer.resize(MAX_FRAMES_IN_FLIGHT);
		m_GlobalDescriptorSet.resize(MAX_FRAMES_IN_FLIGHT);

		m_GlobalDescriptorSetLayout = m_Device->CreateDescriptorSetLayout({
			{0, DescriptorType::UniformBuffer, 1, ShaderStage::Vertex | ShaderStage::Fragment}
			});
		std::vector<RHIDescriptorSetLayoutBinding> bindings;
		// ubo
		bindings.push_back({ .binding = MaterialSlot::Params, .descriptorType = DescriptorType::UniformBuffer, .stageFlags = ShaderStage::Vertex | ShaderStage::Fragment });
		// pbr textures
		bindings.push_back({ .binding = MaterialSlot::BaseColor, .descriptorType = DescriptorType::CombinedImageSampler, .stageFlags = ShaderStage::Fragment });
		bindings.push_back({ .binding = MaterialSlot::Normal, .descriptorType = DescriptorType::CombinedImageSampler, .stageFlags = ShaderStage::Fragment });
		bindings.push_back({ .binding = MaterialSlot::MetallicRoughness, .descriptorType = DescriptorType::CombinedImageSampler, .stageFlags = ShaderStage::Fragment });
		bindings.push_back({ .binding = MaterialSlot::Occlusion, .descriptorType = DescriptorType::CombinedImageSampler, .stageFlags = ShaderStage::Fragment });
		bindings.push_back({ .binding = MaterialSlot::Emissive, .descriptorType = DescriptorType::CombinedImageSampler, .stageFlags = ShaderStage::Fragment });
		m_PbrDescriptorSetLayout = m_Device->CreateDescriptorSetLayout(bindings);
		for (uint8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			m_GlobalDescriptorSet[i] = m_Device->CreateDescriptorSet(m_GlobalDescriptorSetLayout);
			m_GlobalUniformBuffer[i] = m_Device->CreateBuffer(sizeof(SceneData), BufferUsage::UniformBuffer, true);
			m_GlobalDescriptorSet[i]->UpdateBuffer(0, *m_GlobalUniformBuffer[i]);
			m_RenderContext->globalDescriptorSet.push_back(m_GlobalDescriptorSet[i].get());
		}

		m_RenderContext->device = m_Device.get();
		m_RenderContext->swapchain = m_Swapchain.get();
		m_RenderContext->window = m_Window;
		m_RenderContext->MAX_FRAMES_IN_FLIGHT = MAX_FRAMES_IN_FLIGHT;
		m_RenderContext->globalDescriptorSetLayout = m_GlobalDescriptorSetLayout.get();
		m_RenderContext->globalPbrDescriptorSetLayout = m_PbrDescriptorSetLayout.get();
		m_RenderContext->useTextureCompression = true;

		m_Resource = std::make_unique<Resource>(m_RenderContext);

		m_UIPass = std::make_unique<UIPass>();
		m_UIPass->Setup(m_RenderContext);
		m_PbrPass = std::make_unique<PbrPass>();
		m_PbrPass->Setup(m_RenderContext);
		if (OitEnabled)
		{
			m_OitPass = std::make_unique<OitPass>();
			m_OitPass->Setup(m_RenderContext);
		}
		// m_OitPass = std::make_unique<OitPass>();
		// m_OitPass->Setup(m_RenderContext);
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
		return false; // Returning false to propagate the event further
	}
	bool Renderer::OnKeyPress()
	{
		if (Input::IsKeyPressed(Key::X))
		{
			OitEnabled = !OitEnabled;
			BEAR_CORE_INFO("OIT: {}", OitEnabled ? "ON" : "OFF");
			return true;
			// std::cout << "OIT: " << (OitEnabled ? "ON" : "OFF") << std::endl;
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