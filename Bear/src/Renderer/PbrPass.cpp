#include "bearpch.h"
#include "PbrPass.h"

namespace Bear
{
	PbrPass::~PbrPass()
	{
	}

	void PbrPass::Setup(RenderContext* context)
	{
		m_Context = context;
		CreateRenderPass();
		CreateFramebuffers();
		CreatePbrDescriptorSetLayout();
		CreatePipeline();
	}

	void PbrPass::Execute(RHICommandList* cmd, std::vector<RenderObject> renderObjects)
	{
		auto currentImageIndex = m_Context->device->GetCurrentImageIndex();
		auto currentFrameIndex = m_Context->device->GetCurrentFrameIndex();
		auto width = m_Context->swapchain->GetWidth();
		auto height = m_Context->swapchain->GetHeight();
		std::vector<RHIClearValue> clearValues = { {{0.1f, 0.1f, 0.1f}, {}, false}, {{}, {}, true} };
		cmd->BeginRenderPass(*m_RenderPass, *m_Framebuffers[currentImageIndex], width, height, clearValues);
		cmd->SetScissor(0, 0, width, height);
		cmd->SetViewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		for (const auto& obj : renderObjects)
		{
			cmd->BindPipeline(*m_PreZPipeline);
			cmd->BindDescriptorSet(*m_PreZPipelineLayout, *m_Context->globalDescriptorSet[currentFrameIndex], 0);
			cmd->BindDescriptorSet(*m_PreZPipelineLayout, *obj.material->GetDescriptorSet(), 1);
			PerObjectPushConstants pushConstants{ .model = obj.transform };
			cmd->PushConstants(*m_PreZPipelineLayout, ShaderStage::Vertex, &pushConstants, sizeof(PerObjectPushConstants), 0);
			obj.mesh->Bind(*cmd);
			obj.mesh->Draw(*cmd);
		}
		cmd->NextSubpass();
		for (const auto& obj : renderObjects)
		{
			cmd->BindPipeline(*m_PbrPipeline);
			cmd->BindDescriptorSet(*m_PbrPipelineLayout, *m_Context->globalDescriptorSet[currentFrameIndex], 0);
			cmd->BindDescriptorSet(*m_PbrPipelineLayout, *obj.material->GetDescriptorSet(), 1);
			PerObjectPushConstants pushConstants{ .model = obj.transform };
			cmd->PushConstants(*m_PbrPipelineLayout, ShaderStage::Vertex, &pushConstants, sizeof(PerObjectPushConstants), 0);
			obj.mesh->Bind(*cmd);
			obj.mesh->Draw(*cmd);
		}
		cmd->EndRenderPass();
	}

	void PbrPass::Resize()
	{
		m_Context->device->WaitIdle();
		Cleanup();
		CreateFramebuffers();
	}

	void PbrPass::Cleanup()
	{
		m_Framebuffers.clear();
	}

	void PbrPass::CreateRenderPass()
	{
		RenderPassDescription desc;
		desc.attachmentCount = 2; // 1 for depth, 1 for color
		AttachmentDescription& colorAttachment = desc.attachments[0];
		colorAttachment.format = PixelFormat::B8G8R8A8_SRGB;
		colorAttachment.samples = AttachmentSamples::Count1;
		colorAttachment.loadOp = AttachmentLoadOp::Clear;
		colorAttachment.storeOp = AttachmentStoreOp::Store;
		colorAttachment.initialLayout = ImageLayout::Undefined;
		colorAttachment.finalLayout = ImageLayout::ColorAttachment;

		AttachmentDescription& depthAttachment = desc.attachments[1];
		depthAttachment.format = PixelFormat::D32_SFLOAT;
		depthAttachment.samples = AttachmentSamples::Count1;
		depthAttachment.loadOp = AttachmentLoadOp::Clear;
		depthAttachment.storeOp = AttachmentStoreOp::Store;
		depthAttachment.initialLayout = ImageLayout::Undefined;
		depthAttachment.finalLayout = ImageLayout::DepthStencilAttachment;

		desc.subpassCount = 2; // One for pre-Z pass, one for PBR pass
		SubpassDescription& preZSubpass = desc.subpasses[0];
		preZSubpass.hasDepthStencil = true;
		preZSubpass.depthStencilAttachment = {1, ImageLayout::DepthStencilAttachment};

		SubpassDescription& pbrSubpass = desc.subpasses[1];
		pbrSubpass.colorAttachmentCount = 1;
		pbrSubpass.colorAttachments[0] = {0, ImageLayout::ColorAttachment};
		pbrSubpass.hasDepthStencil = true;
		pbrSubpass.depthStencilAttachment = { 1, ImageLayout::DepthStencilAttachment };

		desc.dependencyCount = 1;
		SubpassDependency& dependency = desc.dependencies[0];
		dependency.srcSubpass = 0;
		dependency.dstSubpass = 1;
		dependency.srcStageMask = PipelineStage::LateFragmentTests | PipelineStage::EarlyFragmentTests;
		dependency.dstStageMask = PipelineStage::LateFragmentTests | PipelineStage::EarlyFragmentTests;
		dependency.srcAccessMask = AccessFlags::DepthStencilAttachmentWrite;
		dependency.dstAccessMask = AccessFlags::DepthStencilAttachmentRead;

		m_RenderPass = m_Context->device->CreateRenderPass(desc);
	}

	void PbrPass::CreateFramebuffers()
	{
		uint32_t width = m_Context->swapchain->GetWidth();
		uint32_t height = m_Context->swapchain->GetHeight();
		auto depthAttachment = m_Context->swapchain->GetDepthView();
		// auto colorAttachment = m_Context->swapchain->GetC();
		for (uint32_t i = 0; i < m_Context->swapchain->GetImageCount(); ++i)
		{
			m_Framebuffers.push_back(m_Context->device->CreateFramebuffer(*m_RenderPass, { m_Context->swapchain->GetColorView(i), depthAttachment }, width, height));
		}
	}

	void PbrPass::CreatePipeline()
	{
		std::vector<RHIPushConstantRange> pushConstantRanges = {
			{ShaderStage::Vertex, sizeof(PerObjectPushConstants), 0}
		};
		const auto& globalDescriptorSetLayout = m_Context->globalDescriptorSetLayout;
		m_PreZPipelineLayout = m_Context->device->CreatePipelineLayout({ globalDescriptorSetLayout, m_DescriptorSetLayout.get()},
		                                                              { pushConstantRanges });

		RHIPipelineConfig pipelineConfig;
		pipelineConfig.pipelineLayout = m_PreZPipelineLayout;
		pipelineConfig.vertexShaderPath = "assets/shaders/preZvert.spv";
		pipelineConfig.fragmentShaderPath = "assets/shaders/preZfrag.spv";
		pipelineConfig.colorBlendAttachmentState.colorWriteMask = ColorWriteMask::None;
		pipelineConfig.depthStencilState.depthTestEnable = true;
		pipelineConfig.depthStencilState.depthWriteEnable = true;
		pipelineConfig.depthStencilState.depthCompareOp = CompareOp::Less;
		pipelineConfig.subpassIndex = 0;
		m_PreZPipeline = m_Context->device->CreatePipeline(pipelineConfig, *m_RenderPass);

		m_PbrPipelineLayout = m_Context->device->CreatePipelineLayout({ globalDescriptorSetLayout, m_DescriptorSetLayout.get()},
			{ pushConstantRanges });

		pipelineConfig.pipelineLayout = m_PbrPipelineLayout;
		pipelineConfig.vertexShaderPath = "assets/shaders/pbrVert.spv";
		pipelineConfig.fragmentShaderPath = "assets/shaders/pbrFrag.spv";
		pipelineConfig.colorBlendAttachmentState.colorWriteMask = ColorWriteMask::All;
		pipelineConfig.depthStencilState.depthWriteEnable = false;
		pipelineConfig.depthStencilState.depthCompareOp = CompareOp::Equal;
		pipelineConfig.subpassIndex = 1;
		m_PbrPipeline = m_Context->device->CreatePipeline(pipelineConfig, *m_RenderPass);
	}
	void PbrPass::CreatePbrDescriptorSetLayout()
	{
		std::vector<RHIDescriptorSetLayoutBinding> bindings;
		// ubo
		bindings.push_back({ .binding = MaterialSlot::Params, .descriptorType = DescriptorType::UniformBuffer, .stageFlags = ShaderStage::Vertex | ShaderStage::Fragment });
		// pbr textures
		bindings.push_back({ .binding = MaterialSlot::BaseColor, .descriptorType = DescriptorType::CombinedImageSampler, .stageFlags = ShaderStage::Fragment });
		bindings.push_back({ .binding = MaterialSlot::Normal, .descriptorType = DescriptorType::CombinedImageSampler, .stageFlags = ShaderStage::Fragment });
		bindings.push_back({ .binding = MaterialSlot::MetallicRoughness, .descriptorType = DescriptorType::CombinedImageSampler, .stageFlags = ShaderStage::Fragment });
		bindings.push_back({ .binding = MaterialSlot::Occlusion, .descriptorType = DescriptorType::CombinedImageSampler, .stageFlags = ShaderStage::Fragment });
		bindings.push_back({ .binding = MaterialSlot::Emissive, .descriptorType = DescriptorType::CombinedImageSampler, .stageFlags = ShaderStage::Fragment });

		m_DescriptorSetLayout = m_Context->device->CreateDescriptorSetLayout(bindings);
	}
}
