#include "bearpch.h"

#include "VulkanValidation.h"
#include "VulkanDevice.h"
#include "Bear/Log.h"
#include "VulkanInstance.h"
#include "Presentation/VulkanSurface.h"
#include "VulkanBuffer.h"
#include "Pipeline/VulkanDescriptorSetLayout.h"
#include "Core/VulkanUtils.h"
#include "Pipeline/VulkanPipelineLayout.h"
#include "Pipeline/VulkanPipeline.h"
#include "Pipeline/VulkanDescriptorSet.h"
#include "Pipeline/VulkanDescriptorPool.h"
#include "Pipeline/VulkanRenderPass.h"
#include "Pipeline/VulkanShader.h"
#include "Presentation/VulkanSwapchain.h"
#include "Command/VulkanCommandPool.h"
#include "Command/VulkanCommandBuffer.h"
#include "Sync/VulkanFence.h"
#include "Sync/VulkanSemaphore.h"
namespace Bear {

	Bear::VulkanDevice::VulkanDevice(GLFWwindow* window)
	{
		m_Instance = std::make_unique<VulkanInstance>("app", "engine", true);
		m_Surface = std::make_unique<VulkanSurface>(*m_Instance, window);
		PickPhysicalDevice(m_Instance->GetHandle(), m_Surface->GetHandle());
		CreateLogicalDevice(m_Instance->GetHandle(), m_Surface->GetHandle());
#ifdef BEAR_DEBUG
		BEAR_CORE_INFO("Vulkan Device created successfully.");
#endif // BEAR_DEBUG

		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_0;
		allocatorInfo.physicalDevice = m_PhysicalDevice;
		allocatorInfo.device = m_LogicalDevice;
		allocatorInfo.instance = m_Instance->GetHandle();
		vmaCreateAllocator(&allocatorInfo, &m_Allocator); // vma

		BEAR_CORE_ASSERT(m_Allocator != VK_NULL_HANDLE, "Failed to create VMA allocator!");
		// ****************这里的实现不太好********************
		std::vector<VkDescriptorPoolSize> globalPoolSizes = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 }
		// 未来可以添加更多类型，比如 StorageBuffer
		};
		m_GlobalDescriptorPool = std::make_unique<VulkanDescriptorPool>(*this, 1000, globalPoolSizes);

		m_CommandPool = std::make_unique<VulkanCommandPool>(*this);
		m_CommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			m_CommandBuffers[i] = std::make_unique<VulkanCommandBuffer>(*m_CommandPool);
			m_ImageAvailableSemaphores[i] = std::make_unique<VulkanSemaphore>(*this);
			m_RenderFinishedSemaphores[i] = std::make_unique<VulkanSemaphore>(*this);
			m_InFlightFences[i] = std::make_unique<VulkanFence>(*this, true);
		}
	}

	VulkanDevice::~VulkanDevice()
	{
		if (m_Allocator != VK_NULL_HANDLE)
		{
			vmaDestroyAllocator(m_Allocator);
			m_Allocator = VK_NULL_HANDLE;
#ifdef BEAR_DEBUG
			BEAR_CORE_INFO("VMA Allocator destroyed successfully.");
#endif // BEAR_DEBUG
		}
		m_CommandBuffers.clear();
		m_ImageAvailableSemaphores.clear();
		m_RenderFinishedSemaphores.clear();
		m_InFlightFences.clear();
		m_CommandPool.reset();
		m_GlobalDescriptorPool.reset();

		if (m_LogicalDevice != VK_NULL_HANDLE)
		{
			vkDestroyDevice(m_LogicalDevice, nullptr);
			m_LogicalDevice = VK_NULL_HANDLE;
#ifdef BEAR_DEBUG
			BEAR_CORE_INFO("Vulkan Device destroyed successfully.");
#endif // BEAR_DEBUG
	
		}
		m_Surface.reset();
		m_Instance.reset();
	}
	
	void VulkanDevice::SubmitCommands(RHICommandList& cmd)
	{
		auto imageAvailableSemaphore = m_ImageAvailableSemaphores[m_CurrentFrame]->GetHandle();
		auto renderFinishedSemaphore = m_RenderFinishedSemaphores[m_CurrentFrame]->GetHandle();
		auto inFlightFence = m_InFlightFences[m_CurrentFrame]->GetHandle();

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
		submitInfo.pWaitDstStageMask = waitStages;

		auto& vkCmd = static_cast<VulkanCommandBuffer&>(cmd);
		submitInfo.commandBufferCount = 1;
		auto vkCmdHandle = vkCmd.GetHandle();
		submitInfo.pCommandBuffers = &vkCmdHandle;

		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &renderFinishedSemaphore;

		BEAR_CORE_ASSERT(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, inFlightFence) == VK_SUCCESS, "submit commands failed!");
	}
	std::unique_ptr<RHIBuffer> VulkanDevice::CreateBuffer(size_t size, BufferUsage usage, bool cpuAccessible)
	{
		// 翻译usage
		VkBufferUsageFlags vkUsage = ToVulkanBufferUsage(usage);
		
		VmaMemoryUsage memUsage = cpuAccessible ? VMA_MEMORY_USAGE_CPU_TO_GPU : VMA_MEMORY_USAGE_GPU_ONLY;
		return std::make_unique<VulkanBuffer>(*this, size, vkUsage, memUsage);
	}
	std::shared_ptr<RHIDescriptorSetLayout> VulkanDevice::CreateDescriptorSetLayout(const std::vector<RHIDescriptorSetLayoutBinding>& bindings)
	{
		std::vector<VkDescriptorSetLayoutBinding> vkBindings;
		vkBindings.reserve(bindings.size());
		for (const auto& binding : bindings)
		{
			VkDescriptorSetLayoutBinding vkBinding = {};
			vkBinding.binding = binding.binding;
			vkBinding.descriptorType = ToVulkanDescriptorType(binding.descriptorType);
			vkBinding.descriptorCount = binding.descriptorCount;
			vkBinding.stageFlags = ToVulkanShaderStage(binding.stageFlags);
			vkBinding.pImmutableSamplers = nullptr; // 不可变采样器
			vkBindings.push_back(vkBinding);
		}
		/*
		* make_shared 返回一个 std::shared_ptr<VulkanDescriptorSetLayout>，
        * 它被自动转换为 std::shared_ptr<RHIDescriptorSetLayout> 并返回。
		*/
		return std::make_shared<VulkanDescriptorSetLayout>(*this, vkBindings);
	}
	std::shared_ptr<RHIPipelineLayout> VulkanDevice::CreatePipelineLayout(const std::vector<std::shared_ptr<RHIDescriptorSetLayout>>& descriptorSetLayouts)
	{
		std::vector<VkDescriptorSetLayout> layouts;
		layouts.reserve(descriptorSetLayouts.size());
		for (const auto& layout : descriptorSetLayouts) {
			layouts.push_back(static_cast<const VulkanDescriptorSetLayout*>(layout.get())->GetHandle());
		}
		return std::make_shared<VulkanPipelineLayout>(*this, layouts);
	}
	std::shared_ptr<RHIPipeline> VulkanDevice::CreatePipeline(const RHIPipelineConfig& config, const RHIRenderPass& renderPass)
	{
		const auto& vkRenderPass = static_cast<const VulkanRenderPass&>(renderPass);
		auto vkPipelineLayout = std::static_pointer_cast<VulkanPipelineLayout>(config.pipelineLayout);
		std::vector<std::unique_ptr<VulkanShader>> shaders;
		shaders.push_back(std::make_unique<VulkanShader>(*this, config.vertexShaderPath, VK_SHADER_STAGE_VERTEX_BIT));
		shaders.push_back(std::make_unique<VulkanShader>(*this, config.fragmentShaderPath, VK_SHADER_STAGE_FRAGMENT_BIT));

		std::vector<VkPipelineShaderStageCreateInfo> shaderStageInfos;
		for (const auto& shader : shaders) {
			shaderStageInfos.push_back(shader->GetStageCreateInfo());
		}

		VkVertexInputBindingDescription bindingDescription = Vertex::GetBindingDescription();
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions = Vertex::GetAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
		inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyInfo.topology = ToVulkanTopology(config.topology); // 
		inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo viewportInfo{};
		viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportInfo.viewportCount = 1;
		viewportInfo.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
		rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationInfo.polygonMode = ToVulkanPolygonMode(config.polygonMode);
		rasterizationInfo.cullMode = ToVulkanCullMode(config.cullMode);
		rasterizationInfo.frontFace = ToVulkanFrontFace(config.frontFace);
		rasterizationInfo.lineWidth = 1.0f;

		VkPipelineMultisampleStateCreateInfo multisampleInfo{};
		multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // 默认值
		multisampleInfo.sampleShadingEnable = VK_FALSE; // 默认值

		VkPipelineColorBlendAttachmentState colorBlendAttachment{}; // 默认值
		colorBlendAttachment.blendEnable = VK_FALSE; // 默认值
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT; // 默认值

		VkPipelineColorBlendStateCreateInfo colorBlendInfo{}; // 默认值
		colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendInfo.logicOpEnable = VK_FALSE; // 默认值
		colorBlendInfo.logicOp = VK_LOGIC_OP_COPY; // 默认值
		colorBlendInfo.attachmentCount = 1; // 默认值
		colorBlendInfo.pAttachments = &colorBlendAttachment; // 默认值

		VkPipelineDepthStencilStateCreateInfo depthStencilInfo{}; // 默认值
		depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilInfo.depthTestEnable = VK_TRUE; // 启用深度测试
		depthStencilInfo.depthWriteEnable = VK_TRUE; // 启用深度写入
		depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS; // 深度比较操作
		depthStencilInfo.depthBoundsTestEnable = VK_FALSE; // 禁用深度范围测试
		depthStencilInfo.stencilTestEnable = VK_FALSE; // 禁用模板测试
		depthStencilInfo.front = {}; // 默认值
		depthStencilInfo.back = {}; // 默认值

		std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
		dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicStateInfo.pDynamicStates = dynamicStates.data();

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = static_cast<uint32_t>(shaderStageInfos.size());
		pipelineInfo.pStages = shaderStageInfos.data();
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
		pipelineInfo.pViewportState = &viewportInfo;
		pipelineInfo.pRasterizationState = &rasterizationInfo;
		pipelineInfo.pMultisampleState = &multisampleInfo;
		pipelineInfo.pColorBlendState = &colorBlendInfo;
		pipelineInfo.pDepthStencilState = &depthStencilInfo;
		pipelineInfo.pDynamicState = &dynamicStateInfo;

		pipelineInfo.layout = vkPipelineLayout->GetHandle();
		pipelineInfo.renderPass = vkRenderPass.GetHandle();
		pipelineInfo.subpass = 0; // 我们要使用的子流程索引

		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional
		return std::make_shared<VulkanPipeline>(*this, pipelineInfo);
	}
	std::unique_ptr<RHIDescriptorSet> VulkanDevice::CreateDescriptorSet(std::shared_ptr<RHIDescriptorSetLayout> layout)
	{
		auto vkLayout = std::dynamic_pointer_cast<VulkanDescriptorSetLayout>(layout);
		return std::make_unique<VulkanDescriptorSet>(*this, *vkLayout, *m_GlobalDescriptorPool);
	}
	std::shared_ptr<RHIRenderPass> VulkanDevice::CreateRenderPass(const std::vector<RHIAttachmentDescription>& attachments)
	{
		std::vector<VkAttachmentDescription> vkAttachments;
		vkAttachments.reserve(attachments.size());

		for (const auto& attachment : attachments) {
			VkAttachmentDescription vkAttachment{};
			vkAttachment.format = ToVulkanFormat(attachment.format);
			vkAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // temp
			vkAttachment.loadOp = ToVulkanLoadOp(attachment.loadOp);
			vkAttachment.storeOp = ToVulkanStoreOp(attachment.storeOp);
			vkAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			vkAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			vkAttachment.initialLayout = ToVulkanImageLayout(attachment.initialLayout);
			vkAttachment.finalLayout = ToVulkanImageLayout(attachment.finalLayout);
			vkAttachments.push_back(vkAttachment);
		}

		// 2. 定义子流程 (Subpass) 和附件引用

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0; // 附件数组中的索引
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		// 3. 定义子流程依赖 (Subpass Dependency)
		// 确保在我们可以写入颜色之前，图像已经从呈现引擎转换到适合渲染的布局
		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // 隐含的外部子流程
		dependency.dstSubpass = 0; // 我们的子流程
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		// 4. 创建 Render Pass
		//std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(vkAttachments.size());
		renderPassInfo.pAttachments = vkAttachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		return std::make_shared<VulkanRenderPass>(*this, renderPassInfo);
	}
	std::unique_ptr<RHISwapchain> VulkanDevice::CreateSwapchain(RHIRenderPass& renderPass)
	{
		auto& vkRenderPass = static_cast<VulkanRenderPass&>(renderPass);
		return std::make_unique<VulkanSwapchain>(*this, vkRenderPass);
	}
	RHICommandList& VulkanDevice::BeginFrame()
	{
		m_InFlightFences[m_CurrentFrame]->Wait(); // 等待上一个帧的命令完成

		m_InFlightFences[m_CurrentFrame]->Reset(); // 重置当前帧的信号量
		RHICommandList& cmd = *m_CommandBuffers[m_CurrentFrame];
		cmd.Reset(); // 重置命令缓冲区
		cmd.Begin(); // 开始命令缓冲区的录制
		return cmd;
	}
	void VulkanDevice::EndFrame(RHISwapchain& swapchain, uint32_t imageIndex)
	{
		RHICommandList& cmd = *m_CommandBuffers[m_CurrentFrame];
		cmd.End(); // 结束命令缓冲区的录制

		SubmitCommands(cmd); // 提交命令缓冲区到图形队列

		Present(swapchain, imageIndex); // 提交交换链的呈现请求
	}
	uint32_t VulkanDevice::AcquireNextImage(RHISwapchain& swapchain) const
	{
		auto& vkSwapchain = static_cast<VulkanSwapchain&>(swapchain);
		return vkSwapchain.AcquireNextImage(*m_ImageAvailableSemaphores[m_CurrentFrame]);
	}
	void VulkanDevice::Present(RHISwapchain& swapchain, uint32_t imageIndex)
	{
		auto& vkSwapchain = static_cast<VulkanSwapchain&>(swapchain);
		vkSwapchain.Present(imageIndex, *m_RenderFinishedSemaphores[m_CurrentFrame]);
		m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}
	void VulkanDevice::CreateLogicalDevice(VkInstance instance, VkSurfaceKHR surface)
	{
		QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice, surface);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = {
			m_QueueIndices.graphicsFamily.value(),
			m_QueueIndices.presentFamily.value()
		};
		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}
		VkPhysicalDeviceFeatures deviceFeatures = {};
		deviceFeatures.samplerAnisotropy = VK_TRUE; // 启用各向异性过滤

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount = static_cast<uint32_t>(m_DeviceExtensions.size());
		createInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();

		BEAR_CORE_ASSERT(vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_LogicalDevice) == VK_SUCCESS, "Failed to create logical device!");

		vkGetDeviceQueue(m_LogicalDevice, m_QueueIndices.graphicsFamily.value(), 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_LogicalDevice, m_QueueIndices.presentFamily.value(), 0, &m_PresentQueue);
	}
	void VulkanDevice::PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface)
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		BEAR_CORE_ASSERT(deviceCount, "Failed to find GPUs with Vulkan support!");

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
		for (const auto& device : devices)
		{
			if (IsDeviceSuitable(device, surface))
			{
				m_PhysicalDevice = device;
				m_QueueIndices = FindQueueFamilies(device, surface);
				break;
			}
		}

		BEAR_CORE_ASSERT(m_PhysicalDevice != VK_NULL_HANDLE, "Failed to find a suitable GPU!");
	}
	bool VulkanDevice::IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface)
	{
		QueueFamilyIndices indices = FindQueueFamilies(device, surface);
		bool extensionsSupported = CheckDeviceExtensionSupport(device);

		bool swapChainAdequate = false; // 检查是否支持交换链
		if (extensionsSupported) {
			uint32_t formatCount = 0;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
			if (formatCount > 0) {
				uint32_t presentModeCount = 0;
				vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
				swapChainAdequate = presentModeCount > 0;
			}
		}
		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
		return indices.IsComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
	}
	QueueFamilyIndices VulkanDevice::FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
	{
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		BEAR_CORE_ASSERT(queueFamilyCount > 0, "Failed to find queue families for the physical device!");
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
		QueueFamilyIndices indices;
		for (uint32_t i = 0; i < queueFamilyCount; ++i)
		{
			if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphicsFamily = i;
			}
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
			if (presentSupport)
			{
				indices.presentFamily = i;
			}
			if (indices.IsComplete())
			{
				break;
			}
		}
		return indices;
	}
	bool VulkanDevice::CheckDeviceExtensionSupport(VkPhysicalDevice device)
	{
		uint32_t extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		BEAR_CORE_ASSERT(extensionCount > 0, "No device extensions found!");
		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
		std::set<std::string> requiredExtensions(m_DeviceExtensions.begin(), m_DeviceExtensions.end());

		for (const auto& extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}
		return requiredExtensions.empty();
	}
}