#include "bearpch.h"

#include "Device.h"
#include "Instance.h"
#include "Presentation/Surface.h"
#include "Buffer.h"
#include "Sampler.h"
#include "Pipeline/DescriptorSetLayout.h"
#include "Core/Utils.h"
#include "Pipeline/PipelineLayout.h"
#include "Pipeline/Pipeline.h"
#include "Pipeline/DescriptorSet.h"
#include "Pipeline/DescriptorPool.h"
#include "Pipeline/RenderPass.h"
#include "Pipeline/Shader.h"
#include "Presentation/Swapchain.h"
#include "Command/CommandPool.h"
#include "Command/CommandBuffer.h"
#include "Sync/Fence.h"
#include "Sync/Semaphore.h"
#include "Resources/Image.h"
namespace Bear {

	Bear::Device::Device(GLFWwindow* window)
	{
		m_Instance = std::make_unique<Instance>("app", "engine", true);
		m_Surface = std::make_unique<Surface>(*m_Instance, window);
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
		// ****************�����ʵ�ֲ�̫��********************
		std::vector<VkDescriptorPoolSize> globalPoolSizes = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 }
		// δ���������Ӹ������ͣ����� StorageBuffer
		};
		m_GlobalDescriptorPool = std::make_unique<DescriptorPool>(*this, 1000, globalPoolSizes);

		m_CommandPool = std::make_unique<CommandPool>(*this);
		m_CommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			m_CommandBuffers[i] = std::make_unique<CommandBuffer>(*m_CommandPool);
			m_ImageAvailableSemaphores[i] = std::make_unique<Semaphore>(*this);
			m_RenderFinishedSemaphores[i] = std::make_unique<Semaphore>(*this);
			m_InFlightFences[i] = std::make_unique<Fence>(*this, true);
		}

		m_ImmediateCommandPool = std::make_unique<CommandPool>(*this);
	}

	Device::~Device()
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
		m_ImmediateCommandPool.reset();

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
	
	void Device::SubmitCommands(RHICommandList& cmd)
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

		auto& vkCmd = static_cast<CommandBuffer&>(cmd);
		submitInfo.commandBufferCount = 1;
		auto vkCmdHandle = vkCmd.GetHandle();
		submitInfo.pCommandBuffers = &vkCmdHandle;

		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &renderFinishedSemaphore;

		BEAR_CORE_ASSERT(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, inFlightFence) == VK_SUCCESS, "submit commands failed!");
	}
	std::unique_ptr<RHIBuffer> Device::CreateBuffer(size_t size, BufferUsage usage, bool cpuAccessible)
	{
		// ����usage
		VkBufferUsageFlags vkUsage = ToVulkanBufferUsage(usage);
		
		VmaMemoryUsage memUsage = cpuAccessible ? VMA_MEMORY_USAGE_CPU_TO_GPU : VMA_MEMORY_USAGE_GPU_ONLY;
		return std::make_unique<Buffer>(*this, size, vkUsage, memUsage);
	}
	std::shared_ptr<RHIDescriptorSetLayout> Device::CreateDescriptorSetLayout(const std::vector<RHIDescriptorSetLayoutBinding>& bindings)
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
			vkBinding.pImmutableSamplers = nullptr; // ���ɱ������
			vkBindings.push_back(vkBinding);
		}
		/*
		* make_shared ����һ�� std::shared_ptr<DescriptorSetLayout>��
        * �����Զ�ת��Ϊ std::shared_ptr<RHIDescriptorSetLayout> �����ء�
		*/
		return std::make_shared<DescriptorSetLayout>(*this, vkBindings);
	}
	std::shared_ptr<RHIPipelineLayout> Device::CreatePipelineLayout(const std::vector<RHIDescriptorSetLayout*>& descriptorSetLayouts, const std::vector<RHIPushConstantRange>& pushConstantRanges)
	{
		std::vector<DescriptorSetLayout*> layouts;
		layouts.reserve(descriptorSetLayouts.size());
		for (const auto& layout : descriptorSetLayouts) {
			layouts.push_back((dynamic_cast<DescriptorSetLayout*>(layout)));
		}
		std::vector<VkPushConstantRange> vkPushConstantRanges;
		for (const auto& range : pushConstantRanges) {
			BEAR_CORE_ASSERT(range.size <= 128, "Push constant size must not exceed 128 bytes!");
			vkPushConstantRanges.push_back(ToVulkanPushConstantRange(range));
		}
		return std::make_shared<PipelineLayout>(*this, layouts, vkPushConstantRanges);
	}
	std::shared_ptr<RHIPipeline> Device::CreatePipeline(const RHIPipelineConfig& config, const RHIRenderPass& renderPass)
	{
		const auto& vkRenderPass = dynamic_cast<const RenderPass&>(renderPass);
		auto vkPipelineLayout = std::static_pointer_cast<PipelineLayout>(config.pipelineLayout);
		std::vector<std::unique_ptr<Shader>> shaders;
		shaders.push_back(std::make_unique<Shader>(*this, config.vertexShaderPath, VK_SHADER_STAGE_VERTEX_BIT));
		shaders.push_back(std::make_unique<Shader>(*this, config.fragmentShaderPath, VK_SHADER_STAGE_FRAGMENT_BIT));

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
		multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // Ĭ��ֵ
		multisampleInfo.sampleShadingEnable = VK_FALSE; // Ĭ��ֵ

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.blendEnable = config.colorBlendAttachmentState.blendEnable ? VK_TRUE : VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = ToVulkanBlendFactor(config.colorBlendAttachmentState.srcColorBlendFactor);
		colorBlendAttachment.dstColorBlendFactor = ToVulkanBlendFactor(config.colorBlendAttachmentState.dstColorBlendFactor);
		colorBlendAttachment.colorBlendOp = ToVulkanBlendOp(config.colorBlendAttachmentState.colorBlendOp);
		colorBlendAttachment.srcAlphaBlendFactor = ToVulkanBlendFactor(config.colorBlendAttachmentState.srcAlphaBlendFactor);
		colorBlendAttachment.dstAlphaBlendFactor = ToVulkanBlendFactor(config.colorBlendAttachmentState.dstAlphaBlendFactor);
		colorBlendAttachment.alphaBlendOp = ToVulkanBlendOp(config.colorBlendAttachmentState.alphaBlendOp);
		colorBlendAttachment.colorWriteMask = ToVulkanColorWriteMask(config.colorBlendAttachmentState.colorWriteMask);

		VkPipelineColorBlendStateCreateInfo colorBlendInfo{}; // Ĭ��ֵ
		colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendInfo.logicOpEnable = VK_FALSE; // Ĭ��ֵ
		colorBlendInfo.logicOp = VK_LOGIC_OP_COPY; // Ĭ��ֵ
		colorBlendInfo.attachmentCount = 1; // Ĭ��ֵ
		colorBlendInfo.pAttachments = &colorBlendAttachment; // Ĭ��ֵ

		VkPipelineDepthStencilStateCreateInfo depthStencilInfo{}; // Ĭ��ֵ
		depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilInfo.depthTestEnable = config.depthStencilState.depthTestEnable ? VK_TRUE : VK_FALSE;
		depthStencilInfo.depthWriteEnable = config.depthStencilState.depthWriteEnable ? VK_TRUE : VK_FALSE;
		depthStencilInfo.depthCompareOp = ToVulkanCompareOp(config.depthStencilState.depthCompareOp);
		depthStencilInfo.depthBoundsTestEnable = VK_FALSE; // ������ȷ�Χ����
		depthStencilInfo.stencilTestEnable = VK_FALSE; // ����ģ�����
		depthStencilInfo.front = {}; // Ĭ��ֵ
		depthStencilInfo.back = {}; // Ĭ��ֵ

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
		pipelineInfo.subpass = config.subpassIndex;

		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional
		return std::make_shared<Pipeline>(*this, pipelineInfo);
	}
	std::unique_ptr<RHIDescriptorSet> Device::CreateDescriptorSet(std::shared_ptr<RHIDescriptorSetLayout> layout)
	{
		auto vkLayout = std::dynamic_pointer_cast<DescriptorSetLayout>(layout);
		return std::make_unique<DescriptorSet>(*this, *vkLayout, *m_GlobalDescriptorPool);
	}
	std::shared_ptr<RHIRenderPass> Device::CreateRenderPass(const std::vector<RHIAttachmentDescription>& attachments)
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

		// 2. ���������� (Subpass) �͸�������

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0; // ���������е�����
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		std::array<VkSubpassDescription, 2> subpasses{};
		// pre-z pass
		subpasses[0] = {};
		subpasses[0].flags = 0;
		subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpasses[0].colorAttachmentCount = 0;
		subpasses[0].pColorAttachments = nullptr;
		subpasses[0].pDepthStencilAttachment = &depthAttachmentRef;
		// pbr pass
		subpasses[1] = {};
		subpasses[1].flags = 0;
		subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpasses[1].colorAttachmentCount = 1;
		subpasses[1].pColorAttachments = &colorAttachmentRef;
		subpasses[1].pDepthStencilAttachment = &depthAttachmentRef;
		// 3. �������������� (Subpass Dependency)
		// ȷ�������ǿ���д����ɫ֮ǰ��ͼ���Ѿ��ӳ�������ת�����ʺ���Ⱦ�Ĳ���
		VkSubpassDependency dependency{};
		dependency.srcSubpass = 0;
		dependency.dstSubpass = 1;
		dependency.srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		// 4. ���� Render Pass
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(vkAttachments.size());
		renderPassInfo.pAttachments = vkAttachments.data();
		renderPassInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
		renderPassInfo.pSubpasses = subpasses.data();
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		return std::make_shared<RenderPass>(*this, renderPassInfo);
	}
	std::unique_ptr<RHISwapchain> Device::CreateSwapchain(RHIRenderPass& renderPass)
	{
		auto& vkRenderPass = static_cast<RenderPass&>(renderPass);
		return std::make_unique<Swapchain>(*this, vkRenderPass);
	}
	std::unique_ptr<RHIImage> Device::CreateTexture(const RHITextureConfig& config)
	{
		VkFormat vkFormat = ToVulkanFormat(config.format);
		VkImageUsageFlags vkUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		return std::make_unique<Image>(*this, config.width, config.height, vkFormat, VK_IMAGE_TILING_OPTIMAL, vkUsage, VMA_MEMORY_USAGE_GPU_ONLY);
	}
	std::shared_ptr<RHISampler> Device::CreateSampler(const RHISamplerConfig& config)
	{
		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = ToVulkanFilter(config.magFilter);
		samplerInfo.minFilter = ToVulkanFilter(config.minFilter);
		samplerInfo.addressModeU = ToVulkanAddressMode(config.addressModeU);
		samplerInfo.addressModeV = ToVulkanAddressMode(config.addressModeV);
		samplerInfo.addressModeW = ToVulkanAddressMode(config.addressModeW);
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);
		samplerInfo.anisotropyEnable = config.anisotropyEnable ? VK_TRUE : VK_FALSE;
		samplerInfo.maxAnisotropy = std::min(config.maxAnisotropy, properties.limits.maxSamplerAnisotropy);
		samplerInfo.borderColor = ToVulkanBorderColor(config.borderColor);
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = config.compareEnable ? VK_TRUE : VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS; 
		samplerInfo.mipmapMode = ToVulkanMipmapMode(config.mipmapMode); 
		samplerInfo.minLod = 0.f; 
		samplerInfo.maxLod = VK_LOD_CLAMP_NONE; 

		return std::make_shared<Sampler>(*this, samplerInfo);
	}
	std::shared_ptr<RHIRenderPass> Device::CreateUIRenderPass()
	{
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = VK_FORMAT_B8G8R8A8_SRGB;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		return std::make_shared<RenderPass>(*this, renderPassInfo);
	}
	std::vector<std::shared_ptr<RHIFramebuffer>> Device::CreateUIFramebuffer(RHIRenderPass& renderPass, RHISwapchain& swapchain)
	{
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		auto& vkRenderPass = static_cast<RenderPass&>(renderPass);
		auto& vkSwapchain = static_cast<Swapchain&>(swapchain);
		framebufferInfo.renderPass = vkRenderPass.GetHandle();
		framebufferInfo.attachmentCount = 1; // 1 attachment for color
		framebufferInfo.pAttachments = nullptr; // Will be set later
		framebufferInfo.width = vkSwapchain.GetWidth();
		framebufferInfo.height = vkSwapchain.GetHeight();
		framebufferInfo.layers = 1; // No layers for single-layer framebuffer

		std::vector<std::shared_ptr<RHIFramebuffer>> framebuffers;
		for (uint32_t i = 0; i < vkSwapchain.GetImageCount(); ++i) {
			auto imageView = vkSwapchain.GetImageViews()[i];
			framebufferInfo.pAttachments = &imageView;
			framebuffers.push_back(std::make_shared<Framebuffer>(*this, framebufferInfo));
		}
		return framebuffers;
	}
	
	void Device::ImmediateSubmit(std::function<void(RHICommandList&)>&& function)
	{
		std::unique_ptr<CommandBuffer> cmd = std::make_unique<CommandBuffer>(*m_ImmediateCommandPool);
		cmd->Begin();
		function(*cmd);
		cmd->End();
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		auto cmdHandle = cmd->GetHandle();
		submitInfo.pCommandBuffers = &cmdHandle;

		vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

		vkQueueWaitIdle(m_GraphicsQueue);
		cmd.reset();
	}
	RHICommandList* Device::BeginFrame()
	{
		m_InFlightFences[m_CurrentFrame]->Wait(); // �ȴ���һ��֡���������

		m_InFlightFences[m_CurrentFrame]->Reset(); // ���õ�ǰ֡���ź���
		RHICommandList* cmd = m_CommandBuffers[m_CurrentFrame].get();
		cmd->Reset(); // �����������
		cmd->Begin(); // ��ʼ���������¼��
		return cmd;
	}
	void Device::EndFrame(RHISwapchain& swapchain, uint32_t imageIndex)
	{
		RHICommandList& cmd = *m_CommandBuffers[m_CurrentFrame];
		cmd.End(); // �������������¼��

		SubmitCommands(cmd); // �ύ���������ͼ�ζ���

		Present(swapchain, imageIndex); // �ύ�������ĳ�������
	}
	uint32_t Device::AcquireNextImage(RHISwapchain& swapchain)
	{
		auto& vkSwapchain = static_cast<Swapchain&>(swapchain);
		return  m_CurrentImageIndex = vkSwapchain.AcquireNextImage(*m_ImageAvailableSemaphores[m_CurrentFrame]);
	}
	void Device::Present(RHISwapchain& swapchain, uint32_t imageIndex)
	{
		auto& vkSwapchain = static_cast<Swapchain&>(swapchain);
		vkSwapchain.Present(imageIndex, *m_RenderFinishedSemaphores[m_CurrentFrame]);
		m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}
	void Device::CreateLogicalDevice(VkInstance instance, VkSurfaceKHR surface)
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
		deviceFeatures.samplerAnisotropy = VK_TRUE; // ���ø������Թ���

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
	void Device::PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface)
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
	bool Device::IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface)
	{
		QueueFamilyIndices indices = FindQueueFamilies(device, surface);
		bool extensionsSupported = CheckDeviceExtensionSupport(device);

		bool swapChainAdequate = false; // ����Ƿ�֧�ֽ�����
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
	QueueFamilyIndices Device::FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
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
	bool Device::CheckDeviceExtensionSupport(VkPhysicalDevice device)
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