#include "bearpch.h"
#include "HiZPass.h"
#include "RHI/RHIDevice.h"
#include "RHI/RHIResources.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIPipeline.h"

namespace Bear
{
	namespace
	{
		constexpr uint32_t kGroupSize = 8;

		size_t MipSize(size_t base, uint32_t level)
		{
			size_t s = base >> level;
			return s > 0 ? s : 1;
		}
	}

	HiZPass::~HiZPass() = default;

	void HiZPass::Setup(RenderContext* context)
	{
		m_Context = context;
		m_DepthWidth = context->swapchain->GetWidth();
		m_DepthHeight = context->swapchain->GetHeight();

		CreatePyramids();
		CreatePipeline();
	}

	void HiZPass::CreatePyramids()
	{
		uint32_t baseWidth = (m_DepthWidth + 1) / 2;
		uint32_t baseHeight = (m_DepthHeight + 1) / 2;

		m_MipCount = 1;
		while ((baseWidth >> m_MipCount) > 0 || (baseHeight >> m_MipCount) > 0)
			++m_MipCount;

		RHITextureConfig config;
		config.width = baseWidth;
		config.height = baseHeight;
		config.format = PixelFormat::R32_SFLOAT;
		config.usage = ImageUsage::Storage | ImageUsage::Sampled;
		config.mipLevels = m_MipCount;

		for (uint32_t i = 0; i < 2; ++i)
		{
			m_Pyramids[i] = m_Context->device->CreateTexture(config);
		}

		RHISamplerConfig samplerConfig;
		samplerConfig.magFilter = Filter::Nearest;
		samplerConfig.minFilter = Filter::Nearest;
		samplerConfig.mipmapMode = MipmapMode::Nearest;
		samplerConfig.addressModeU = SamplerAddressMode::ClampToEdge;
		samplerConfig.addressModeV = SamplerAddressMode::ClampToEdge;
		samplerConfig.addressModeW = SamplerAddressMode::ClampToEdge;
		samplerConfig.anisotropyEnable = false;
		m_Sampler = m_Context->device->CreateSampler(samplerConfig);

		// one-time transition: UNDEFINED -> GENERAL (the pyramid lives in GENERAL for its lifetime)
		m_Context->device->ImmediateSubmit([&](RHICommandList& cmd)
		{
			for (uint32_t i = 0; i < 2; ++i)
			{
				ImageBarrierDesc barrier;
				barrier.srcStageMask = PipelineStage::TopOfPipe;
				barrier.dstStageMask = PipelineStage::AllCommands;
				barrier.srcAccessMask = AccessFlags::None;
				barrier.dstAccessMask = AccessFlags::ShaderRead | AccessFlags::ShaderWrite;
				barrier.oldLayout = ImageLayout::Undefined;
				barrier.newLayout = ImageLayout::General;
				barrier.baseMipLevel = 0;
				barrier.levelCount = m_MipCount;
				cmd.ImageBarrier(*m_Pyramids[i], barrier);
			}
		});

		m_DescriptorSetLayout = m_Context->device->CreateDescriptorSetLayout({
			{0, DescriptorType::CombinedImageSampler, 1, ShaderStage::Compute },
			{1, DescriptorType::StorageImage, 1, ShaderStage::Compute },
			});

		m_DescriptorSets.clear();
		m_DescriptorSets.resize(m_Context->MAX_FRAMES_IN_FLIGHT);
		for (uint8_t frame = 0; frame < m_Context->MAX_FRAMES_IN_FLIGHT; ++frame)
		{
			m_DescriptorSets[frame].resize(m_MipCount);
			for (uint32_t level = 0; level < m_MipCount; ++level)
			{
				m_DescriptorSets[frame][level] = m_Context->device->CreateDescriptorSet(m_DescriptorSetLayout);
			}
		}
		m_LastBuiltFrame = UINT32_MAX;
	}

	void HiZPass::CreatePipeline()
	{
		m_PipelineLayout = m_Context->device->CreatePipelineLayout(
			{ m_DescriptorSetLayout.get() },
			{ { ShaderStage::Compute, sizeof(glm::ivec2), 0 } });

		RHIPipelineConfig config;
		config.pipelineLayout = m_PipelineLayout;
		config.computeShaderPath = "Bear/src/Shaders/hiz.spv";
		m_Pipeline = m_Context->device->CreateComputePipeline(config);
	}

	void HiZPass::Resize()
	{
		uint32_t width = m_Context->swapchain->GetWidth();
		uint32_t height = m_Context->swapchain->GetHeight();
		if (width == m_DepthWidth && height == m_DepthHeight)
			return;

		m_Context->device->WaitIdle();
		m_DepthWidth = width;
		m_DepthHeight = height;
		CreatePyramids();
	}

	void HiZPass::Execute(RHICommandList* cmd, RHIImage& depthImage, uint32_t frameIndex)
	{
		cmd->BindPipeline(*m_Pipeline);

		// only rewrite descriptors on the first build of this frame; the second build
		// (complete-depth refresh) reuses the same sets, which are already bound in the
		// command buffer being recorded
		const bool updateDescriptors = (m_LastBuiltFrame != frameIndex);
		m_LastBuiltFrame = frameIndex;

		auto& descriptorSets = m_DescriptorSets[frameIndex];

		size_t srcWidth = MipSize(m_DepthWidth, 0);
		size_t srcHeight = MipSize(m_DepthHeight, 0);

		for (uint32_t level = 0; level < m_MipCount; ++level)
		{
			auto& set = descriptorSets[level];

			if (updateDescriptors)
			{
				if (level == 0)
				{
					set->UpdateSampledImage(0, depthImage, *m_Sampler, 0, ImageLayout::ShaderReadOnly);
				}
				else
				{
					set->UpdateSampledImage(0, *m_Pyramids[frameIndex], *m_Sampler, level - 1, ImageLayout::General);
				}
				set->UpdateStorageImage(1, *m_Pyramids[frameIndex], level);
			}

			size_t dstWidth = MipSize((m_DepthWidth + 1) / 2, level);
			size_t dstHeight = MipSize((m_DepthHeight + 1) / 2, level);

			cmd->BindDescriptorSet(*m_PipelineLayout, *set, 0, PipelineBindPoint::Compute);

			glm::ivec2 srcSize{ static_cast<int>(srcWidth), static_cast<int>(srcHeight) };
			cmd->PushConstants(*m_PipelineLayout, ShaderStage::Compute, &srcSize, sizeof(srcSize), 0);

			uint32_t groupsX = static_cast<uint32_t>((dstWidth + kGroupSize - 1) / kGroupSize);
			uint32_t groupsY = static_cast<uint32_t>((dstHeight + kGroupSize - 1) / kGroupSize);
			cmd->Dispatch(groupsX, groupsY, 1);

			// make this level visible to the next level's sampling
			if (level + 1 < m_MipCount)
			{
				MemoryBarrier barrier;
				barrier.srcStageMask = PipelineStage::ComputeShader;
				barrier.dstStageMask = PipelineStage::ComputeShader;
				barrier.srcAccessMask = AccessFlags::ShaderWrite;
				barrier.dstAccessMask = AccessFlags::ShaderRead;
				cmd->PipelineBarrier(barrier);
			}

			srcWidth = dstWidth;
			srcHeight = dstHeight;
		}

		// N.B. the pyramid write -> culling read barrier is emitted by the frame graph
	}

	void HiZPass::Cleanup()
	{
		m_Pipeline.reset();
		m_PipelineLayout.reset();
		m_DescriptorSetLayout.reset();
		m_DescriptorSets.clear();
		m_Pyramids[0].reset();
		m_Pyramids[1].reset();
		m_Sampler.reset();
	}
}
