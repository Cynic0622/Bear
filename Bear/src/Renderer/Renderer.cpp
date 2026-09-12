#include "bearpch.h"
#include "Renderer.h"
#include "CommandBuffer.h"
#include "DescriptorSet.h"
#include "Shader.h"
#include "UIPass.h"
#include "PbrPass.h"
#include "OitPass.h"
#include "CullingPass.h"
#include "HiZPass.h"
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

		size_t perObjectSize = renderObjects.size() * sizeof(glm::mat4);
		auto& perObjBuf = m_PerObjectBuffer[frameIndex];
		if (perObjBuf->GetSize() < perObjectSize)
		{
			m_Device->WaitIdle();
			perObjBuf = m_Device->CreateBuffer(perObjectSize + sizeof(glm::mat4) * 256,
				BufferUsage::StorageBuffer, true);
			m_SceneDataDescriptorSet[frameIndex]->UpdateBuffer(2, *perObjBuf);
		}
		for (size_t i = 0; i < renderObjects.size(); ++i)
		{
			perObjBuf->UploadData(&renderObjects[i].transform, sizeof(glm::mat4), i * sizeof(glm::mat4));
		}
		// ---------------- frame graph flow ----------------
		m_RenderGraph.Reset();

		// swapchain targets enter the graph as Undefined: the graph emits the explicit
		// layout transitions required by the dynamic-rendering passes
		const auto colorTarget = m_RenderGraph.ImportTexture("color", m_Swapchain->GetColorImage(m_CurrentImageIndex), ImageLayout::Undefined);
		const auto depthTarget = m_RenderGraph.ImportTexture("depth", m_Swapchain->GetDepthImage(m_CurrentImageIndex), ImageLayout::Undefined);

		const bool occlusionActive = m_HiZPass && m_CullingPass->IsOcclusionActive();

		// resources participating in the graph (render targets keep their layout management
		// inside their render passes for now)
		RenderGraph::TextureHandle hizPrev; // invalid until imported
		RenderGraph::TextureHandle hizCur;
		if (m_HiZPass)
		{
			uint32_t prevSlot = (frameIndex + 1) % 2; // ping-pong of 2 pyramid slots
			hizPrev = m_RenderGraph.ImportTexture("hiZ.prev", m_HiZPass->GetPyramid(prevSlot), ImageLayout::General);
			hizCur = m_RenderGraph.ImportTexture("hiZ.cur", m_HiZPass->GetPyramid(frameIndex), ImageLayout::General);
		}

		const auto bufferA = m_RenderGraph.ImportBuffer("cull.outA", m_CullingPass->GetOutputCommandsBuffer());
		const auto countersA = m_RenderGraph.ImportBuffer("cull.countersA", m_CullingPass->GetCountersBuffer());
		const auto bufferB = m_RenderGraph.ImportBuffer("cull.outB", m_CullingPass->GetRescuedCommandsBuffer());
		const auto countersB = m_RenderGraph.ImportBuffer("cull.countersB", m_CullingPass->GetRescuedCountersBuffer());

		m_CullingPass->SetViewProjection(baseData.projMat * baseData.viewMat);
		if (m_HiZPass)
		{
			uint32_t prevSlot = (frameIndex + 1) % 2;
			m_CullingPass->SetHiZSource(m_HiZPass->GetPyramid(prevSlot), m_HiZPass->GetSampler());
			m_CullingPass->SetHiZMipCount(m_HiZPass->GetMipCount());
		}

		// Skybox: clears the swapchain color/depth through dynamic rendering
		m_RenderGraph.AddPass("Skybox",
			[&](RenderGraph::PassBuilder& b)
			{
				b.Write(colorTarget, { PipelineStage::ColorAttachmentOutput, AccessFlags::ColorAttachmentWrite, ImageLayout::ColorAttachment });
				b.Write(depthTarget, { PipelineStage::EarlyFragmentTests | PipelineStage::LateFragmentTests, AccessFlags::DepthStencilAttachmentWrite, ImageLayout::DepthStencilAttachment });
			},
			[&](RHICommandList& cmd)
			{
				m_SkyboxPass->Execute(&cmd, m_Swapchain->GetColorImage(m_CurrentImageIndex), m_Swapchain->GetDepthImage(m_CurrentImageIndex));
			});

		// Culling phase 1 -> visible set A
		m_RenderGraph.AddPass("Culling.Primary",
			[&](RenderGraph::PassBuilder& b)
			{
				b.Read(hizPrev, { PipelineStage::ComputeShader, AccessFlags::ShaderRead, ImageLayout::General });
				b.Write(bufferA, { PipelineStage::ComputeShader, AccessFlags::ShaderWrite });
				b.Write(countersA, { PipelineStage::ComputeShader, AccessFlags::ShaderWrite });
			},
			[this, renderObjects](RHICommandList& cmd) { m_CullingPass->Execute(&cmd, renderObjects); });

		// PBR stage 1: draws set A with cleared depth
		m_PbrPass->ResetStats();
		m_RenderGraph.AddPass("PBR.Primary",
			[&](RenderGraph::PassBuilder& b)
			{
				b.Read(bufferA, { PipelineStage::DrawIndirect, AccessFlags::IndirectCommandRead });
				b.Read(countersA, { PipelineStage::DrawIndirect, AccessFlags::IndirectCommandRead });
				b.Write(colorTarget, { PipelineStage::ColorAttachmentOutput, AccessFlags::ColorAttachmentWrite, ImageLayout::ColorAttachment });
				b.Write(depthTarget, { PipelineStage::EarlyFragmentTests | PipelineStage::LateFragmentTests, AccessFlags::DepthStencilAttachmentWrite, ImageLayout::DepthStencilAttachment });
			},
			[&](RHICommandList& cmd)
			{
				m_PbrPass->Execute(&cmd, m_CullingPass->GetVisibleSet(), true,
					m_Swapchain->GetColorImage(m_CurrentImageIndex), m_Swapchain->GetDepthImage(m_CurrentImageIndex));
			});

		if (occlusionActive)
		{
			// Hi-Z build from PBR stage-1 depth
			m_RenderGraph.AddPass("HiZ.Build",
				[&](RenderGraph::PassBuilder& b)
				{
					b.Write(hizCur, { PipelineStage::ComputeShader, AccessFlags::ShaderWrite, ImageLayout::General });
					b.Read(depthTarget, { PipelineStage::ComputeShader, AccessFlags::ShaderRead, ImageLayout::ShaderReadOnly });
				},
				[this, frameIndex](RHICommandList& cmd)
				{
					m_HiZPass->Execute(&cmd, *m_Swapchain->GetDepthImage(m_CurrentImageIndex), frameIndex);
				});

			// Culling phase 2: same-frame retest of deferred objects -> set B
			m_RenderGraph.AddPass("Culling.Refine",
				[&](RenderGraph::PassBuilder& b)
				{
					b.Read(hizCur, { PipelineStage::ComputeShader, AccessFlags::ShaderRead, ImageLayout::General });
					b.Write(bufferB, { PipelineStage::ComputeShader, AccessFlags::ShaderWrite });
					b.Write(countersB, { PipelineStage::ComputeShader, AccessFlags::ShaderWrite });
				},
				[this, frameIndex](RHICommandList& cmd)
				{
					m_CullingPass->SetHiZSource(m_HiZPass->GetPyramid(frameIndex), m_HiZPass->GetSampler());
					m_CullingPass->ExecutePhase2(&cmd);
				});

			// PBR stage 2: rescued set B with loaded depth
			m_RenderGraph.AddPass("PBR.Refine",
				[&](RenderGraph::PassBuilder& b)
				{
					b.Read(bufferB, { PipelineStage::DrawIndirect, AccessFlags::IndirectCommandRead });
					b.Read(countersB, { PipelineStage::DrawIndirect, AccessFlags::IndirectCommandRead });
					b.Write(colorTarget, { PipelineStage::ColorAttachmentOutput, AccessFlags::ColorAttachmentWrite, ImageLayout::ColorAttachment });
					b.Write(depthTarget, { PipelineStage::EarlyFragmentTests | PipelineStage::LateFragmentTests, AccessFlags::DepthStencilAttachmentWrite, ImageLayout::DepthStencilAttachment });
				},
				[&](RHICommandList& cmd)
				{
					m_PbrPass->Execute(&cmd, m_CullingPass->GetRescuedSet(), false,
						m_Swapchain->GetColorImage(m_CurrentImageIndex), m_Swapchain->GetDepthImage(m_CurrentImageIndex));
				});
		}

		if (OitEnabled)
		{
			if (!m_OitPass)
			{
				m_OitPass = std::make_unique<OitPass>();
				m_OitPass->Setup(m_RenderContext);
			}

			// OIT storage lives only inside the OIT pass: transient resources owned by the
			// frame graph (aliased with other transients of disjoint lifetime)
			const uint32_t width = m_Swapchain->GetWidth();
			const uint32_t height = m_Swapchain->GetHeight();
			size_t nodeStride = 32; // vec4 + float in std430
			auto oitNodes = m_RenderGraph.CreateBuffer("oit.nodes",
				static_cast<size_t>(width) * height * MAX_OIT_NODES_PER_PIXEL * nodeStride,
				BufferUsage::StorageBuffer | BufferUsage::TransferDstBuffer, false);

			RHITextureConfig counterConfig;
			counterConfig.width = width;
			counterConfig.height = height;
			counterConfig.format = PixelFormat::R32_UINT;
			counterConfig.usage = ImageUsage::Storage | ImageUsage::TransferDst;
			auto oitCounter = m_RenderGraph.CreateTexture("oit.counter", counterConfig);

			// transparent accumulation reads depth and blends into the color target
			m_RenderGraph.AddPass("OIT",
				[&](RenderGraph::PassBuilder& b)
				{
					b.Read(depthTarget, { PipelineStage::EarlyFragmentTests | PipelineStage::LateFragmentTests, AccessFlags::DepthStencilAttachmentRead, ImageLayout::DepthStencilAttachment });
					b.Write(colorTarget, { PipelineStage::ColorAttachmentOutput, AccessFlags::ColorAttachmentWrite, ImageLayout::ColorAttachment });
					b.Write(oitCounter, { PipelineStage::FragmentShader, AccessFlags::ShaderRead | AccessFlags::ShaderWrite, ImageLayout::General });
					b.Write(oitNodes, { PipelineStage::FragmentShader, AccessFlags::ShaderWrite });
				},
				[this, renderObjects, oitNodes, oitCounter](RHICommandList& cmd)
				{
					m_OitPass->Execute(&cmd, renderObjects,
						m_Swapchain->GetColorImage(m_CurrentImageIndex), m_Swapchain->GetDepthImage(m_CurrentImageIndex),
						m_RenderGraph.GetBuffer(oitNodes), m_RenderGraph.GetTexture(oitCounter));
				});
		}

		// UI draws last (executed after ImGui::Render in EndFrame); the present
		// transition hands the color target over to the presentation engine
		m_RenderGraph.AddPass("UI",
			[&](RenderGraph::PassBuilder& b)
			{
				b.Write(colorTarget, { PipelineStage::ColorAttachmentOutput, AccessFlags::ColorAttachmentWrite, ImageLayout::ColorAttachment });
			},
			[&](RHICommandList& cmd)
			{
				m_UIPass->Execute(&cmd, m_Swapchain->GetColorImage(m_CurrentImageIndex));
			});

		m_RenderGraph.AddPass("Present.Transition",
			[&](RenderGraph::PassBuilder& b)
			{
				b.Read(colorTarget, { PipelineStage::BottomOfPipe, AccessFlags::None, ImageLayout::PresentSrc });
			},
			[](RHICommandList&) {}, true, true);

		m_RenderGraph.Compile();
		if (m_DumpGraphRequested)
		{
			BEAR_CORE_INFO("Frame graph:\n{}", m_RenderGraph.Dump());
			m_DumpGraphRequested = false;
		}

		m_FrameStats.visibleObjects = static_cast<uint32_t>(renderObjects.size());
	}
	
	void Renderer::EndFrame()
	{
		ImGui::Render();
		m_RenderGraph.Execute(*m_CurrentCommandBuffer);

		const auto& pbrStats = m_PbrPass->GetStats();
		m_FrameStats.opaqueObjects = pbrStats.opaqueObjects;
		m_FrameStats.drawCalls = 1 /*skybox*/ + pbrStats.drawCalls;
		const auto& gpuStats = m_CullingPass->GetGpuStats();
		m_FrameStats.gpuVisible = gpuStats.visibleA;
		m_FrameStats.gpuRejected = gpuStats.rejected;
		m_FrameStats.gpuRescued = gpuStats.rescued;
		if (OitEnabled)
		{
			const auto& oitStats = m_OitPass->GetStats();
			m_FrameStats.transparentObjects = oitStats.transparentObjects;
			m_FrameStats.drawCalls += oitStats.drawCalls;
		}

		m_Device->EndFrame(*m_Swapchain, m_CurrentImageIndex);
	}
	void Renderer::Init(GraphicsAPI api)
	{
		RHIPlatformData platformData{};
		platformData.windowHandle = m_Window;

		m_Device = CreateDevice(api, platformData);
		m_RenderGraph.Setup(m_Device.get(), MAX_FRAMES_IN_FLIGHT);

		m_Swapchain = m_Device->CreateSwapchain();

		m_BaseDataUniformBuffer.resize(MAX_FRAMES_IN_FLIGHT);
		m_BaseDataDescriptorSet.resize(MAX_FRAMES_IN_FLIGHT);
		m_SceneDataUniformBuffer.resize(MAX_FRAMES_IN_FLIGHT);
		m_SceneDataDescriptorSet.resize(MAX_FRAMES_IN_FLIGHT);

		m_BaseDataDescriptorSetLayout = m_Device->CreateDescriptorSetLayout({
			{0, DescriptorType::UniformBuffer, 1, ShaderStage::Vertex | ShaderStage::Fragment | ShaderStage::Compute}
			});
		m_SceneDataDescriptorSetLayout = m_Device->CreateDescriptorSetLayout({
			{0, DescriptorType::UniformBuffer, 1, ShaderStage::Vertex | ShaderStage::Fragment},
			{1, DescriptorType::CombinedImageSampler, 1, ShaderStage::Fragment },
			{2, DescriptorType::StorageBuffer, 1, ShaderStage::Vertex | ShaderStage::Compute }
			});

		std::string filePath = "assets/skybox/kloppenheim_06_puresky_4k.hdr";
		m_IBLTexture = std::make_shared<Texture>(*m_Device, filePath);
		m_IBLDescriptorSet = m_Device->CreateDescriptorSet(m_SceneDataDescriptorSetLayout);
		m_IBLDescriptorSet->UpdateTexture(1, m_IBLTexture->GetImage(), m_IBLTexture->GetSampler());

		m_PerObjectBuffer.resize(MAX_FRAMES_IN_FLIGHT);
		for (uint8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			m_BaseDataDescriptorSet[i] = m_Device->CreateDescriptorSet(m_BaseDataDescriptorSetLayout);
			m_BaseDataUniformBuffer[i] = m_Device->CreateBuffer(sizeof(BaseData), BufferUsage::UniformBuffer, true);
			m_BaseDataDescriptorSet[i]->UpdateBuffer(0, *m_BaseDataUniformBuffer[i]);
			m_SceneDataDescriptorSet[i] = m_Device->CreateDescriptorSet(m_SceneDataDescriptorSetLayout);
			m_SceneDataUniformBuffer[i] = m_Device->CreateBuffer(sizeof(SceneData), BufferUsage::UniformBuffer, true);
			m_SceneDataDescriptorSet[i]->UpdateBuffer(0, *m_SceneDataUniformBuffer[i]);
			m_SceneDataDescriptorSet[i]->UpdateTexture(1, m_IBLTexture->GetImage(), m_IBLTexture->GetSampler());
			m_PerObjectBuffer[i] = m_Device->CreateBuffer(65536 * sizeof(glm::mat4), BufferUsage::StorageBuffer, true);
			m_SceneDataDescriptorSet[i]->UpdateBuffer(2, *m_PerObjectBuffer[i]);
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
		m_CullingPass = std::make_unique<CullingPass>();
		m_CullingPass->Setup(m_RenderContext);
		m_HiZPass = std::make_unique<HiZPass>();
		m_HiZPass->Setup(m_RenderContext);
	}
	
	
	bool Renderer::IsGpuCullingEnabled() const
	{
		return m_CullingPass && m_CullingPass->IsEnabled();
	}
	bool Renderer::IsOcclusionCullingEnabled() const
	{
		return m_CullingPass && m_CullingPass->IsOcclusionEnabled();
	}
	bool Renderer::OnWindowResize() const
	{
		if (m_Swapchain) {
			m_Swapchain->Resize();
		}
		if (m_HiZPass)
		{
			m_HiZPass->Resize();
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
		if (Input::IsKeyPressed(Key::C))
		{
			m_CullingPass->SetEnabled(!m_CullingPass->IsEnabled());
			BEAR_CORE_INFO("GPU Culling: {}", m_CullingPass->IsEnabled() ? "ON" : "OFF");
			return true;
		}
		if (Input::IsKeyPressed(Key::O))
		{
			bool enable = !m_CullingPass->IsOcclusionEnabled();
			m_CullingPass->SetOcclusionEnabled(enable);
			if (enable && !m_CullingPass->IsEnabled())
			{
				// occlusion culling lives in the GPU culling pipeline: enable it together
				m_CullingPass->SetEnabled(true);
				BEAR_CORE_INFO("Occlusion Culling: ON (GPU Culling auto-enabled)");
			}
			else
			{
				BEAR_CORE_INFO("Occlusion Culling: {}", enable ? "ON" : "OFF");
			}
			return true;
		}
		if (Input::IsKeyPressed(Key::G))
		{
			m_DumpGraphRequested = true;
			BEAR_CORE_INFO("Dumping frame graph next frame...");
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