#include "bearpch.h"
#include "UIPass.h"

#include "Application.h"
#include "CommandBuffer.h"
#include "Swapchain.h"
#include "RHI/RHIRenderPass.h"

namespace Bear
{
	UIPass::~UIPass()
	{
		if (!m_Context->device) return;

		auto* vkDevice = dynamic_cast<Device*>(m_Context->device);
		vkDevice->WaitIdle();

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		Cleanup();
		if (m_DescriptorPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(vkDevice->GetDevice(), m_DescriptorPool, nullptr);
			m_DescriptorPool = VK_NULL_HANDLE;
		}
		
	}
	void UIPass::Setup(RenderContext* context)
	{
		m_Context = context;
		m_RenderPass = m_Context->device->CreateUIRenderPass();
		m_Framebuffers = m_Context->device->CreateUIFramebuffer(*m_RenderPass, *m_Context->swapchain);
		InitGUI();
	}

	void UIPass::Execute(RHICommandList* cmd, std::vector<RenderObject> renderObjects)
	{
		uint32_t imageIndex = m_Context->device->GetCurrentImageIndex();
		ImDrawData* drawData = ImGui::GetDrawData();

		cmd->BeginRenderPass(*m_RenderPass, *m_Framebuffers[imageIndex], m_Context->swapchain->GetWidth(), m_Context->swapchain->GetHeight(), {});
		
		ImGui_ImplVulkan_RenderDrawData(drawData, static_cast<VkCommandBuffer>(cmd->GetNativeHandle()));
		
		cmd->EndRenderPass();
	}

	void UIPass::Resize()
	{
		m_Context->device->WaitIdle();
		auto* vkDevice = dynamic_cast<Device*>(m_Context->device);

		m_Framebuffers.clear();
		m_Framebuffers = m_Context->device->CreateUIFramebuffer(*m_RenderPass, *m_Context->swapchain);
	}

	void UIPass::Cleanup()
	{
		m_Framebuffers.clear();
	}

	void UIPass::InitGUI()
	{
		// 1. initialize ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		// 2. IO input
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiBackendFlags_HasMouseCursors; // Enable Mouse Cursors
		io.ConfigFlags |= ImGuiBackendFlags_HasSetMousePos; // Enable SetMousePos backend function

		// set the store path for ImGui settings
		io.IniFilename = "build/imgui.ini";
		// 3. initialize GLFW
		ImGui_ImplGlfw_InitForVulkan(m_Context->window, true);
		
		auto* vkDevice = dynamic_cast<Device*>(m_Context->device);

		// 4. create descriptor pool for ImGui
		VkDescriptorPoolSize pool_sizes[] = {
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};

		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000;
		pool_info.poolSizeCount = std::size(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;

		VkResult result = vkCreateDescriptorPool(vkDevice->GetDevice(), &pool_info, nullptr, &m_DescriptorPool);
		BEAR_CORE_ASSERT(result == VK_SUCCESS, "Failed to create descriptor pool for ImGui!");

		// 8. initialize ImGui Vulkan
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = vkDevice->GetInstance().GetHandle();
		init_info.PhysicalDevice = vkDevice->GetPhysicalDevice();
		init_info.Device = vkDevice->GetDevice();
		init_info.QueueFamily = vkDevice->GetQueueFamilyIndices().graphicsFamily.value();
		init_info.Queue = vkDevice->GetGraphicsQueue();
		init_info.PipelineCache = VK_NULL_HANDLE;
		init_info.DescriptorPool = m_DescriptorPool;
		init_info.RenderPass = static_cast<VkRenderPass>(m_RenderPass->GetNativeHandle());
		init_info.Subpass = 0;
		init_info.MinImageCount = m_Context->swapchain->GetImageCount();
		init_info.ImageCount = m_Context->swapchain->GetImageCount();
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		init_info.Allocator = nullptr;

		ImGui_ImplVulkan_Init(&init_info);
	}
} // namespace Bear
