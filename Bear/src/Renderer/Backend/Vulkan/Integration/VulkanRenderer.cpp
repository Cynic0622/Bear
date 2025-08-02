#include "bearpch.h"

#include "VulkanRenderer.h"
#include "Core/VulkanInstance.h"
#include "Presentation/VulkanSurface.h"
#include "Core/VulkanDevice.h"
#include "Presentation/VulkanSwapchain.h"
#include "Pipeline/VulkanRenderPass.h"
#include "Pipeline/VulkanPipeline.h"
#include "Command/VulkanCommandPool.h"
#include "Command/VulkanCommandBuffer.h"
#include "Sync/VulkanSemaphore.h"	
#include "Sync/VulkanFence.h"
#include <filesystem>
#include "Resources/VulkanBuffer.h"
#include "Core/VulkanTypes.h"
#include "Renderer/Common/Mesh.h"
#include "Renderer/Common/Material.h"
#include "Renderer/Common/RenderObject.h"
#include "RHI/RHITypes.h"
#include "Pipeline/VulkanFramebuffer.h"
namespace Bear {

	VulkanRenderer::VulkanRenderer(GLFWwindow* window)
		:m_Window(window)
	{
		Initialize();
		
	}

	VulkanRenderer::~VulkanRenderer()
	{
		Cleanup();
	}

	void VulkanRenderer::DrawFrame()
	{
		// wait fence
		m_InFlightFences[m_CurrentFrame]->Wait();
		uint32_t imageIndex;
		VkResult result = m_Swapchain->AcquireNextImage(&imageIndex, m_ImageAvailableSemaphores[m_CurrentFrame]->GetHandle());

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			RecreateSwapchain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("Failed to acquire swap chain image!");
		}

		m_InFlightFences[m_CurrentFrame]->Reset();
		m_CommandBuffers[m_CurrentFrame]->Reset();

		UniformBufferObject ubo{};
		ubo.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		VkExtent2D extent = m_Swapchain->GetExtent();
		ubo.proj = glm::perspective(glm::radians(45.0f), (float)extent.width / (float)extent.height, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;

		RecordCommandBuffer(imageIndex, ubo);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame]->GetHandle() };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		VkCommandBuffer cmdBufferHandle = m_CommandBuffers[m_CurrentFrame]->GetHandle();
		submitInfo.pCommandBuffers = &cmdBufferHandle;

		VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame]->GetHandle() };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		BEAR_CORE_ASSERT(vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &submitInfo, m_InFlightFences[m_CurrentFrame]->GetHandle()) == VK_SUCCESS,
			"Failed to submit draw command buffer!");

		result = m_Swapchain->SubmitImage(imageIndex, m_Device->GetPresentQueue(), m_RenderFinishedSemaphores[m_CurrentFrame]->GetHandle());

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FramebufferResized) {
			m_FramebufferResized = false;
			RecreateSwapchain();
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to present swap chain image!");
		}

		m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	}

	void VulkanRenderer::OnWindowResized()
	{
		m_FramebufferResized = true;
		if (m_Swapchain) {
			RecreateSwapchain();
		}
		else {
			BEAR_CORE_ERROR("Swapchain is not initialized, cannot recreate it on window resize.");
		}
	}

	void VulkanRenderer::Initialize()
	{
		m_Instance = std::make_unique<VulkanInstance>("Sandbox", "Bear", true);
		m_Surface = std::make_unique<VulkanSurface>(*m_Instance, m_Window);

		m_Device = std::make_unique<VulkanDevice>(*m_Instance, *m_Surface);

		m_Swapchain = std::make_unique<VulkanSwapchain>(*m_Device, *m_Surface);

		m_RenderPass = std::make_unique<VulkanRenderPass>(*m_Device, *m_Swapchain);
		m_Swapchain->CreateFramebuffers(*m_RenderPass);

		//m_Pipeline = std::make_unique<VulkanPipeline>(*m_Device, *m_RenderPass, *m_Swapchain);
		//VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		BEAR_CORE_ASSERT(vkCreatePipelineLayout(m_Device->GetDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout) == VK_SUCCESS, "Failed to create pipeline layout!");
		
		//std::vector<std::unique_ptr<VulkanShader>> shaders;

		////std::cout << std::filesystem::current_path() << std::endl;
		//shaders.push_back(std::make_unique<VulkanShader>(*m_Device, "../../../Bear/src/Bear/Shaders/tri.vert.spv", VK_SHADER_STAGE_VERTEX_BIT));
		//shaders.push_back(std::make_unique<VulkanShader>(*m_Device, "../../../Bear/src/Bear/Shaders/tri.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT));

		//PipelineConfigInfo configInfo{};
		//PipelineConfigInfo::GetDefaultConfig(configInfo);
		//configInfo.renderPass = m_RenderPass->GetHandle();
		//configInfo.pipelineLayout = m_PipelineLayout;

		//m_Pipeline = std::make_unique<VulkanPipeline>(*m_Device, shaders, configInfo);
		// 定义顶点数据
		std::vector<Vertex> vertices = {
			{{0.5f, 0.5f, 0.f}, {0.0f, 1.0f, 0.0f}},
			{{-0.5f, 0.5f, 0.f}, {0.0f, 0.0f, 1.0f}},
			{{-0.5f, -0.5f, 0.f}, {1.0f, 1.0f, 0.0f}},
			{{0.5f, -0.5f, 0.f}, {1.0f, 0.0f, 1.0f}},
		};
		std::vector<uint16_t> indices = {
			0, 1, 2,
			2, 3, 0
		};

		m_SquareMesh = std::make_shared<Mesh>(*m_Device, vertices, indices);
		std::vector<std::string> shaderPaths = {
			"../../../Bear/src/Bear/Shaders/tri.vert.spv",
			"../../../Bear/src/Bear/Shaders/tri.frag.spv"
		};
		m_SimpleMaterial = std::make_shared<Material>(*m_Device, *m_RenderPass, shaderPaths);
		auto square1 = RenderObject::Create(m_SquareMesh, m_SimpleMaterial);
		square1->transform.translation = { -0.5f, 0.0f, 0.0f }; // 移动到左边
		square1->transform.scale = { 1.0f, 1.0f, 1.0f };
		m_RenderObjects.push_back(std::move(square1));

		auto square2 = RenderObject::Create(m_SquareMesh, std::make_shared<Material>(*m_Device, *m_RenderPass, shaderPaths));
		square2->transform.translation = { 0.5f, 0.0f, 0.0f }; // 移动到右边
		square2->transform.scale = { 0.5f, 0.5f, 0.5f }; // 缩小一半
		m_RenderObjects.push_back(std::move(square2));

		m_CommandPool = std::make_unique<VulkanCommandPool>(*m_Device);

		m_CommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			m_CommandBuffers[i] = std::make_unique<VulkanCommandBuffer>(*m_CommandPool);
		}

		m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			m_ImageAvailableSemaphores[i] = std::make_unique<VulkanSemaphore>(*m_Device);
			m_RenderFinishedSemaphores[i] = std::make_unique<VulkanSemaphore>(*m_Device);
			m_InFlightFences[i] = std::make_unique<VulkanFence>(*m_Device, true); // 创建为 signaled 状态
		}

	}
	void VulkanRenderer::Cleanup()
	{
		m_Device->WaitIdle();
		//m_VertexBuffer.reset(); // 清理顶点缓冲区
		//m_IndexBuffer.reset(); // 清理索引缓冲区
		m_SquareMesh.reset(); // 清理 Mesh
		m_SimpleMaterial.reset(); // 清理材质
		m_RenderObjects.clear(); // 清理渲染对象
		m_Swapchain.reset();
		vkDestroyPipelineLayout(m_Device->GetDevice(), m_PipelineLayout, nullptr);
		m_Pipeline.reset();
		m_RenderPass.reset();

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			m_ImageAvailableSemaphores[i].reset();
			m_RenderFinishedSemaphores[i].reset();
			m_InFlightFences[i].reset();
		}
		m_CommandPool.reset(); // 清理命令池和命令缓冲区

		m_Device.reset();

		m_Surface.reset();

		m_Instance.reset();

	}
	void VulkanRenderer::RecreateSwapchain()
	{
		int width, height;
		glfwGetFramebufferSize(m_Window, &width, &height);
		if (width == 0 || height == 0) {
			return; // 窗口大小无效，跳过重建
		}
		m_Device->WaitIdle(); // 等待设备空闲，确保没有正在使用的资源
		m_Swapchain->Recreate();
		m_Swapchain->CreateFramebuffers(*m_RenderPass);
	}
	void VulkanRenderer::RecordCommandBuffer(uint32_t imageIndex, UniformBufferObject& ubo)
	{
		auto cmd = m_CommandBuffers[m_CurrentFrame].get();
		cmd->Begin();
		std::vector<RHIClearValue> clearValues = { {{}, {}, false}, {{}, {}, true} };
		// temp
		cmd->BeginRenderPass(m_RenderPass.get(), m_Swapchain->GetFramebuffer(imageIndex).GetHandle(), m_Swapchain->GetExtent().width, m_Swapchain->GetExtent().height, clearValues);

		VkExtent2D extent = m_Swapchain->GetExtent();
		cmd->SetViewport(0, 0, (float)extent.width, (float)extent.height, 0.0f, 1.0f);
		cmd->SetScissor(0, 0, extent.width, extent.height);

		for (const auto& obj : m_RenderObjects) {
			// 1. 更新此对象的 Model 矩阵并上传 UBO
			ubo.model = obj->transform.GetTransform();
			obj->material->UpdateUniformBuffer(m_CurrentFrame, ubo);
			// 2. 绑定此对象的材质 (管线和描述符)
			obj->material->Bind(*cmd, m_CurrentFrame);

			// 3. 绑定此对象的几何体并绘制
			obj->mesh->Bind(*cmd);
			obj->mesh->Draw(*cmd);
		}

		//cmd->BindPipeline(*m_Pipeline);
		/*cmd->BindVertexBuffers(0, { m_VertexBuffer.get()}, { 0 });
		cmd->BindIndexBuffer(*m_IndexBuffer);*/
		//m_SquareMesh->Bind(*cmd); // temp, should be removed later
		//cmd->Draw(3, 1, 0, 0); // temp
		//cmd->DrawIndexed(6);
		cmd->EndRenderPass();
		cmd->End();
	}
}