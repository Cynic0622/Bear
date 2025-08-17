#include "bearpch.h"
#include "Renderer.h"

#include "DescriptorSet.h"
#include "Shader.h"
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

	void Renderer::Submit(const std::vector<RenderObject>& renderObjects, const SceneData& sceneData)
	{
		auto frameIndex = m_Device->GetCurrentFrameIndex();
		m_GlobalParams.cameraPosition = glm::vec4(sceneData.cameraPosition, 1.0f);
		m_GlobalParams.viewMatrix = sceneData.viewMatrix;
		m_GlobalParams.projectionMatrix = sceneData.projectionMatrix;
		m_GlobalUniformBuffer[frameIndex]->UploadData(&m_GlobalParams, sizeof(globalParams)); // set 0.
		m_GlobalDescriptorSet[frameIndex]->UpdateBuffer(0, *m_GlobalUniformBuffer[frameIndex]);

		for (const auto& obj : renderObjects) {
			
			BEAR_CORE_ASSERT(obj.material, "RenderObject has no material!");
			// set pipelineLayout
			if (!m_Pipeline)
			{
				std::vector<RHIDescriptorSetLayout*> descriptorSetLayouts = { m_GlobalDescriptorSetLayout[frameIndex].get(), obj.material->GetDescriptorSetLayout()};
				std::vector<RHIPushConstantRange> pushConstantRanges = { {ShaderStage::Vertex, sizeof(PerObjectPushConstants), 0} };
				m_PipelineLayout = m_Device->CreatePipelineLayout(descriptorSetLayouts, pushConstantRanges);

				RHIPipelineConfig pipelineConfig;
				pipelineConfig.pipelineLayout = m_PipelineLayout;
				pipelineConfig.vertexShaderPath = "assets/shaders/vert.spv";
				pipelineConfig.fragmentShaderPath = "assets/shaders/frag.spv";
				m_Pipeline = m_Device->CreatePipeline(pipelineConfig, *m_RenderPass);
			}
			m_CurrentCommandBuffer->BindPipeline(*m_Pipeline);
			// Bind descriptor sets
			// auto descriptorSet = obj.material->GetDescriptorSet();
			// auto vkDescriptorSet = dynamic_cast<DescriptorSet*>(descriptorSet);
			// BEAR_CORE_INFO("Binding DescriptorSet: {:#x}", (uint64_t)vkDescriptorSet->GetHandle());
			obj.material->UpdateParams();
			m_CurrentCommandBuffer->BindDescriptorSet(*m_PipelineLayout, *m_GlobalDescriptorSet[frameIndex], 0);
			m_CurrentCommandBuffer->BindDescriptorSet(*m_PipelineLayout, *obj.material->GetDescriptorSet(), 1);
			// push constants
			PerObjectPushConstants pushConstants{ .model = obj.transform };
			m_CurrentCommandBuffer->PushConstants(*m_PipelineLayout, ShaderStage::Vertex, &pushConstants, sizeof(PerObjectPushConstants), 0);
			//obj.material->Bind(*m_CurrentCommandBuffer, frameIndex);
			obj.mesh->Bind(*m_CurrentCommandBuffer);
			//PerObjectPushConstants pushConstants{ .model = obj.transform };
			//m_CurrentCommandBuffer->PushConstants(*obj.material->GetPipelineLayout(), ShaderStage::Vertex, &pushConstants, sizeof(PerObjectPushConstants), 0);
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
		m_MeshManager = std::make_unique<MeshManager>();
		m_Resource = std::make_unique<Resource>(*m_Device);

		m_GlobalUniformBuffer.resize(MAX_FRAMES_IN_FLIGHT);
		m_GlobalDescriptorSet.resize(MAX_FRAMES_IN_FLIGHT);
		m_GlobalDescriptorSetLayout.resize(MAX_FRAMES_IN_FLIGHT);
		for (uint8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			m_GlobalDescriptorSetLayout[i] = m_Device->CreateDescriptorSetLayout({{0, DescriptorType::UniformBuffer, 1, ShaderStage::Vertex | ShaderStage::Fragment}});
			m_GlobalDescriptorSet[i] = m_Device->CreateDescriptorSet(m_GlobalDescriptorSetLayout[i]);
			m_GlobalUniformBuffer[i] = m_Device->CreateBuffer(sizeof(globalParams), BufferUsage::UniformBuffer, true);
		}

		// randomly generate ten lights in the scene.
		std::srand(static_cast<unsigned>(std::time(nullptr)));
		for (int i = 0; i < m_GlobalParams.lightCount - 1; ++i)
		{
			m_GlobalParams.lightPositions[i] = glm::vec4(
				static_cast<float>(std::rand() % 1000 - 500),
				static_cast<float>(std::rand() % 1000),
				static_cast<float>(std::rand() % 1000 - 500),
				1.0f); // w = 1.0 (可当作标志或不使用)
			m_GlobalParams.lightColors[i] = glm::vec4(
				static_cast<float>(std::rand() % 100) / 99.0f,
				static_cast<float>(std::rand() % 100) / 99.0f,
				static_cast<float>(std::rand() % 100) / 99.0f,
				1.0f);
		}
		m_GlobalParams.lightPositions[9] = glm::vec4(0.f, 5.f, 0.f, 0.f);
		m_GlobalParams.lightColors[9] = glm::vec4(1.f, 1.f, 1.f, 1.f); // last light is white and at origin
		
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