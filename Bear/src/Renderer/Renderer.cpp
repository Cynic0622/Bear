#include "bearpch.h"
#include "Renderer.h"

#include "CommandBuffer.h"
#include "DescriptorSet.h"
#include "Shader.h"
#include "UIPass.h"
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
		std::vector<RHIClearValue> clearValues = { {{0.1f, 0.1f, 0.1f}, {}, false}, {{}, {}, true} };
		m_CurrentCommandBuffer->BeginRenderPass(*m_RenderPass, *m_Swapchain->GetFramebuffer(m_CurrentImageIndex), m_Swapchain->GetWidth(), m_Swapchain->GetHeight(), clearValues);
		uint32_t width, height;
		m_Swapchain->GetExtent(width, height);
		m_CurrentCommandBuffer->SetViewport(0, 0, static_cast<float>(width), static_cast<float>(height));
		m_CurrentCommandBuffer->SetScissor(0, 0, width, height);
		auto frameIndex = m_Device->GetCurrentFrameIndex();
		m_GlobalUniformBuffer[frameIndex]->UploadData(&sceneData, sizeof(sceneData)); // set 0.
		m_GlobalDescriptorSet[frameIndex]->UpdateBuffer(0, *m_GlobalUniformBuffer[frameIndex]);

		for (const auto& obj : renderObjects) {
			
			BEAR_CORE_ASSERT(obj.material, "RenderObject has no material!");
			if (!m_PreZPipeline)
			{
				std::vector<RHIPushConstantRange> pushConstantRanges = { {ShaderStage::Vertex, sizeof(PerObjectPushConstants), 0} };
				m_PreZPipelineLayout = m_Device->CreatePipelineLayout({ m_GlobalDescriptorSetLayout[frameIndex].get(), obj.material->GetDescriptorSetLayout() }, pushConstantRanges);

				RHIPipelineConfig preZConfig;
				preZConfig.pipelineLayout = m_PreZPipelineLayout;
				preZConfig.vertexShaderPath = "assets/shaders/preZvert.spv";
				preZConfig.fragmentShaderPath = "assets/shaders/preZfrag.spv";
				preZConfig.colorBlendAttachmentState.colorWriteMask = ColorWriteMask::None;  // 不写任何颜色通道
				preZConfig.depthStencilState.depthTestEnable = true;
				preZConfig.depthStencilState.depthWriteEnable = true;
				preZConfig.depthStencilState.depthCompareOp = CompareOp::Less;
				preZConfig.subpassIndex = 0; // Pre-Z pass is the first subpass
				m_PreZPipeline = m_Device->CreatePipeline(preZConfig, *m_RenderPass);
			}
			m_CurrentCommandBuffer->BindPipeline(*m_PreZPipeline);
			obj.material->UpdateParams();
			m_CurrentCommandBuffer->BindDescriptorSet(*m_PreZPipelineLayout, *m_GlobalDescriptorSet[frameIndex], 0);
			m_CurrentCommandBuffer->BindDescriptorSet(*m_PreZPipelineLayout, *obj.material->GetDescriptorSet(), 1);
			
			// push constants
			PerObjectPushConstants pushConstants{ .model = obj.transform };
			m_CurrentCommandBuffer->PushConstants(*m_PreZPipelineLayout, ShaderStage::Vertex, &pushConstants, sizeof(PerObjectPushConstants), 0);
			obj.mesh->Bind(*m_CurrentCommandBuffer);
			
			obj.mesh->Draw(*m_CurrentCommandBuffer);
		}
		m_CurrentCommandBuffer->NextSubpass();
		for (const auto& obj : renderObjects) 
		{
			if (!m_PbrPipeline)
			{
				std::vector<RHIDescriptorSetLayout*> descriptorSetLayouts = { m_GlobalDescriptorSetLayout[frameIndex].get(), obj.material->GetDescriptorSetLayout() };
				std::vector<RHIPushConstantRange> pushConstantRanges = { {ShaderStage::Vertex, sizeof(PerObjectPushConstants), 0} };
				m_PipelineLayout = m_Device->CreatePipelineLayout(descriptorSetLayouts, pushConstantRanges);

				RHIPipelineConfig pipelineConfig;
				pipelineConfig.pipelineLayout = m_PipelineLayout;
				pipelineConfig.vertexShaderPath = "assets/shaders/vert.spv";
				pipelineConfig.fragmentShaderPath = "assets/shaders/frag.spv";
				pipelineConfig.depthStencilState.depthTestEnable = true;
				pipelineConfig.depthStencilState.depthWriteEnable = false;
				pipelineConfig.depthStencilState.depthCompareOp = CompareOp::LessOrEqual;
				pipelineConfig.subpassIndex = 1;
				m_PbrPipeline = m_Device->CreatePipeline(pipelineConfig, *m_RenderPass);
			}
			m_CurrentCommandBuffer->BindPipeline(*m_PbrPipeline);

			obj.material->UpdateParams();
			m_CurrentCommandBuffer->BindDescriptorSet(*m_PipelineLayout, *m_GlobalDescriptorSet[frameIndex], 0);
			m_CurrentCommandBuffer->BindDescriptorSet(*m_PipelineLayout, *obj.material->GetDescriptorSet(), 1);

			PerObjectPushConstants pushConstants{ .model = obj.transform };
			m_CurrentCommandBuffer->PushConstants(*m_PipelineLayout, ShaderStage::Vertex, &pushConstants, sizeof(PerObjectPushConstants), 0);
			obj.mesh->Bind(*m_CurrentCommandBuffer);
			obj.mesh->Draw(*m_CurrentCommandBuffer);
		}
		m_CurrentCommandBuffer->EndRenderPass();
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

		RHIAttachmentDescription colorAttachment{};
		colorAttachment.format = PixelFormat::B8G8R8A8_SRGB;
		colorAttachment.loadOp = AttachmentLoadOp::Clear;
		colorAttachment.storeOp = AttachmentStoreOp::Store;
		colorAttachment.initialLayout = ImageLayout::Undefined;
		colorAttachment.finalLayout = ImageLayout::ColorAttachment;

		RHIAttachmentDescription depthAttachment{};
		depthAttachment.format = PixelFormat::D32_SFLOAT;
		depthAttachment.loadOp = AttachmentLoadOp::Clear;
		depthAttachment.storeOp = AttachmentStoreOp::Store;
		depthAttachment.initialLayout = ImageLayout::Undefined;
		depthAttachment.finalLayout = ImageLayout::DepthStencilAttachment;
		m_RenderPass = m_Device->CreateRenderPass({ colorAttachment, depthAttachment });
		// m_UIRenderPass = m_Device->CreateUIRenderPass();
		
		m_Swapchain = m_Device->CreateSwapchain(*m_RenderPass);

		m_RenderContext->device = m_Device.get();
		m_RenderContext->swapchain = m_Swapchain.get();
		m_RenderContext->window = m_Window;
		m_RenderContext->MAX_FRAMES_IN_FLIGHT = MAX_FRAMES_IN_FLIGHT;
		m_UIPass = std::make_unique<UIPass>();
		m_UIPass->Setup(m_RenderContext);
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
			m_GlobalUniformBuffer[i] = m_Device->CreateBuffer(sizeof(SceneData), BufferUsage::UniformBuffer, true);
		}
	}
	
	
	void Renderer::OnWindowResized() const
	{
		if (m_Swapchain) {
			m_Swapchain->Resize();
		}
		if (m_UIPass)
		{
			m_UIPass->Resize();
		}
	}
}