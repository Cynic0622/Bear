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
		RecordCommandBuffer(imageIndex);

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
		
		std::vector<std::unique_ptr<VulkanShader>> shaders;

		std::cout << std::filesystem::current_path() << std::endl;
		shaders.push_back(std::make_unique<VulkanShader>(*m_Device, "../../../Bear/src/Bear/Shaders/tri.vert.spv", VK_SHADER_STAGE_VERTEX_BIT));
		shaders.push_back(std::make_unique<VulkanShader>(*m_Device, "../../../Bear/src/Bear/Shaders/tri.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT));

		PipelineConfigInfo configInfo{};
		PipelineConfigInfo::GetDefaultConfig(configInfo);
		configInfo.renderPass = m_RenderPass->GetHandle();
		configInfo.pipelineLayout = m_PipelineLayout;

		m_Pipeline = std::make_unique<VulkanPipeline>(*m_Device, shaders, configInfo);

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
	void VulkanRenderer::RecordCommandBuffer(uint32_t imageIndex)
	{
		auto cmd = m_CommandBuffers[m_CurrentFrame].get();
		cmd->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		cmd->BeginRenderPass(*m_RenderPass, m_Swapchain->GetFramebuffer(imageIndex), m_Swapchain->GetExtent());

		VkExtent2D extent = m_Swapchain->GetExtent();
		cmd->SetViewport(0, 0, (float)extent.width, (float)extent.height, 0.0f, 1.0f);
		cmd->SetScissor(0, 0, extent.width, extent.height);

		cmd->BindPipeline(*m_Pipeline);

		cmd->Draw(3, 1, 0, 0); // temp

		cmd->EndRenderPass();
		cmd->End();
	}
}