#include "bearpch.h"
#include "ShadowPass.h"
#include "CullingPass.h"
#include "Common/Mesh.h"
#include "RHI/RHIDevice.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIPipeline.h"
#include "RHI/RHIResources.h"

namespace Bear
{
	namespace
	{
		constexpr uint32_t kCullGroupSize = 64;
		constexpr float kSplitLambda = 0.75f;
		// Depth bias in world units; the shader converts it with the per-cascade depth range.
		constexpr float kDepthBiasWorld = 0.1f;
		// The light ortho box has to contain every caster that can shadow the cascade slice, i.e.
		// the geometry between the slice and the light. The slice itself only needs 2*radius in
		// front of the light-space eye, so the near plane is pushed toward the light by as much as
		// the scene's caster bounds require - quantized (so the depth range does not drift every
		// frame), hysteretic (grow at once, shrink only past one step) and capped.
		constexpr float kMaxNearExtendFactor = 4.0f; // in units of the cascade radius
		constexpr float kNearExtendQuantum = 0.25f;  // ditto
		constexpr float kNearPadding = 1.0f;         // world units
		// PCSS penumbra growth: penumbraWorld = (receiver<->blocker gap measured along the light
		// axis, in world units) * kLightSize. Keeping it a function of the world gap only makes the
		// filter radius independent of the camera position (the shader used to normalize by the
		// fitted light-space eye depth, which slid with the camera and made shadows swim).
		constexpr float kLightSize = 0.05f;
		constexpr float kNormalOffset = 1.5f;

		struct ShadowCullPushConstants
		{
			glm::vec4 planes[6];
			uint32_t objectCount;
		};
		static_assert(sizeof(ShadowCullPushConstants) <= 128, "push constants exceed the 128-byte limit");

		struct ShadowRasterPushConstants
		{
			glm::mat4 lightViewProj;
			float texelWorld;
			float normalOffset;
		};
		static_assert(sizeof(ShadowRasterPushConstants) <= 128, "push constants exceed the 128-byte limit");
	}

	void ShadowPass::Setup(RenderContext* context)
	{
		m_Context = context;

		RHISamplerConfig samplerConfig;
		samplerConfig.magFilter = Filter::Nearest;
		samplerConfig.minFilter = Filter::Nearest;
		samplerConfig.mipmapMode = MipmapMode::Nearest;
		samplerConfig.addressModeU = SamplerAddressMode::ClampToEdge;
		samplerConfig.addressModeV = SamplerAddressMode::ClampToEdge;
		samplerConfig.addressModeW = SamplerAddressMode::ClampToEdge;
		samplerConfig.anisotropyEnable = false;
		m_Sampler = m_Context->device->CreateSampler(samplerConfig);

		CreatePipelines();

		m_CommandBuffers.resize(m_Context->MAX_FRAMES_IN_FLIGHT);
		m_CounterBuffers.resize(m_Context->MAX_FRAMES_IN_FLIGHT);
		m_CullSets.resize(m_Context->MAX_FRAMES_IN_FLIGHT);
		for (uint8_t frame = 0; frame < m_Context->MAX_FRAMES_IN_FLIGHT; ++frame)
		{
			for (uint32_t c = 0; c < kCascadeCount; ++c)
			{
				m_CullSets[frame][c] = m_Context->device->CreateDescriptorSet(m_CullSetLayout);
				// host-visible: the counter is also read back for diagnostics
				m_CounterBuffers[frame][c] = m_Context->device->CreateBuffer(
					sizeof(uint32_t), BufferUsage::StorageBuffer | BufferUsage::TransferDstBuffer | BufferUsage::IndirectBuffer, true);
			}
		}
	}

	void ShadowPass::CreatePipelines()
	{
		m_CullSetLayout = m_Context->device->CreateDescriptorSetLayout({
			{0, DescriptorType::StorageBuffer, 1, ShaderStage::Compute }, // AABBs
			{1, DescriptorType::StorageBuffer, 1, ShaderStage::Compute }, // input commands
			{2, DescriptorType::StorageBuffer, 1, ShaderStage::Compute }, // output commands
			{3, DescriptorType::StorageBuffer, 1, ShaderStage::Compute }, // visible counter
			});

		m_CullPipelineLayout = m_Context->device->CreatePipelineLayout(
			{ m_CullSetLayout.get(), m_Context->sceneDataDescriptorSetLayout },
			{ { ShaderStage::Compute, sizeof(ShadowCullPushConstants), 0 } });

		RHIPipelineConfig cullConfig;
		cullConfig.pipelineLayout = m_CullPipelineLayout;
		cullConfig.computeShaderPath = "Bear/src/Shaders/shadowCull.spv";
		m_CullPipeline = m_Context->device->CreateComputePipeline(cullConfig);

		m_RasterPipelineLayout = m_Context->device->CreatePipelineLayout(
			{ m_Context->sceneDataDescriptorSetLayout },
			{ { ShaderStage::Vertex, sizeof(ShadowRasterPushConstants), 0 } });

		RHIPipelineConfig rasterConfig;
		rasterConfig.pipelineLayout = m_RasterPipelineLayout;
		rasterConfig.vertexShaderPath = "Bear/src/Shaders/shadowVert.spv";
		rasterConfig.fragmentShaderPath = "Bear/src/Shaders/shadowFrag.spv";
		rasterConfig.depthStencilState.depthTestEnable = true;
		rasterConfig.depthStencilState.depthWriteEnable = true;
		rasterConfig.depthStencilState.depthCompareOp = CompareOp::LessOrEqual;
		rasterConfig.depthBiasEnable = true;
		rasterConfig.depthBiasConstant = 1.5f;
		rasterConfig.depthBiasSlope = 2.0f;
		rasterConfig.dynamicRendering = true;
		rasterConfig.colorFormats = {}; // depth only
		rasterConfig.depthFormat = PixelFormat::D32_SFLOAT;
		m_RasterPipeline = m_Context->device->CreatePipeline(rasterConfig);
	}

	void ShadowPass::Cleanup()
	{
		m_Sampler.reset();
		m_CullPipeline.reset();
		m_CullPipelineLayout.reset();
		m_CullSetLayout.reset();
		m_RasterPipeline.reset();
		m_RasterPipelineLayout.reset();
		m_CullSets.clear();
		m_CommandBuffers.clear();
		m_CounterBuffers.clear();
	}

	RHITextureConfig ShadowPass::GetShadowMapConfig() const
	{
		RHITextureConfig config;
		config.width = kShadowResolution;
		config.height = kShadowResolution;
		config.format = PixelFormat::D32_SFLOAT;
		config.usage = ImageUsage::DepthStencilAttachment | ImageUsage::Sampled;
		config.arrayLayers = kCascadeCount;
		return config;
	}

	void ShadowPass::UpdateCascades(const glm::mat4& view, const glm::mat4& proj, float nearPlane, float farPlane,
		const glm::vec3& lightDirection, const glm::vec3& casterBoundsMin, const glm::vec3& casterBoundsMax,
		SceneData& sceneData)
	{
		// practical split scheme
		for (uint32_t i = 0; i < kCascadeCount; ++i)
		{
			const float p = static_cast<float>(i + 1) / static_cast<float>(kCascadeCount);
			const float logSplit = nearPlane * std::pow(farPlane / nearPlane, p);
			const float uniformSplit = nearPlane + (farPlane - nearPlane) * p;
			m_Splits[i] = kSplitLambda * logSplit + (1.0f - kSplitLambda) * uniformSplit;
		}

		const glm::mat4 invView = glm::inverse(view);
		const glm::mat4 invProj = glm::inverse(proj);
		const glm::vec3 toLight = -glm::normalize(lightDirection);

		auto unproject = [&](float ndcX, float ndcY, float viewDepth)
		{
			// reconstruct the view-space corner on the ray through this NDC xy at the given view depth
			glm::vec4 viewNear = invProj * glm::vec4(ndcX, ndcY, 0.0f, 1.0f);
			glm::vec4 viewFar = invProj * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
			viewNear /= viewNear.w;
			viewFar /= viewFar.w;

			const float nearDepth = -viewNear.z;
			const float farDepth = -viewFar.z;
			const float t = (viewDepth - nearDepth) / (farDepth - nearDepth);
			glm::vec3 viewPos = glm::mix(glm::vec3(viewNear), glm::vec3(viewFar), t);
			return glm::vec3(invView * glm::vec4(viewPos, 1.0f));
		};

		float prevSplit = nearPlane;
		for (uint32_t c = 0; c < kCascadeCount; ++c)
		{
			const float sliceFar = m_Splits[c];

			glm::vec3 corners[8];
			uint32_t index = 0;
			for (float ndcY : { -1.0f, 1.0f })
			{
				for (float ndcX : { -1.0f, 1.0f })
				{
					corners[index++] = unproject(ndcX, ndcY, prevSplit);
					corners[index++] = unproject(ndcX, ndcY, sliceFar);
				}
			}

			glm::vec3 center(0.0f);
			for (const auto& corner : corners)
				center += corner;
			center /= 8.0f;

			float radius = 0.0f;
			for (const auto& corner : corners)
				radius = std::max(radius, glm::length(corner - center));
			radius = std::max(radius, 1e-4f);

			// stability: quantize the radius so the texel grid scale only changes in steps,
			// then snap the light-space center to whole texels
			radius = std::ceil(radius / 0.5f) * 0.5f;

			const glm::vec3 up = std::abs(toLight.y) > 0.99f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
			const float texelWorld = (2.0f * radius) / static_cast<float>(kShadowResolution);
			const glm::mat4 lightRotation = glm::lookAt(glm::vec3(0.0f), -toLight, up);
			const glm::mat4 invLightRotation = glm::inverse(lightRotation);

			// snap as a small delta: reconstructing a snapped center from large light-space
			// coordinates loses precision and makes the texel grid phase drift (visible swim)
			const glm::vec3 centerLight = glm::vec3(lightRotation * glm::vec4(center, 1.0f));
			const float snappedX = std::round(centerLight.x / texelWorld) * texelWorld;
			const float snappedY = std::round(centerLight.y / texelWorld) * texelWorld;
			const glm::vec3 deltaLight(snappedX - centerLight.x, snappedY - centerLight.y, 0.0f);
			const glm::vec3 snappedCenter = center + glm::vec3(invLightRotation * glm::vec4(deltaLight, 0.0f));

			const glm::vec3 eye = snappedCenter + toLight * radius;
			glm::mat4 lightView = glm::lookAt(eye, snappedCenter, up);

			// Casters that can shadow this slice live between the slice and the light, i.e. on the
			// light side of the light-space eye. Measure how far the scene's caster bounds reach
			// along the light axis (positive = toward the light) and extend the near plane by that
			// much, quantized + hysteretic so the depth range stays put for a static scene.
			float casterReach = -1e30f;
			for (uint32_t corner = 0; corner < 8; ++corner)
			{
				const glm::vec3 p((corner & 1u) ? casterBoundsMax.x : casterBoundsMin.x,
					(corner & 2u) ? casterBoundsMax.y : casterBoundsMin.y,
					(corner & 4u) ? casterBoundsMax.z : casterBoundsMin.z);
				casterReach = std::max(casterReach, glm::dot(p - eye, toLight));
			}
			casterReach = std::max(casterReach, 0.0f); // never cut into the slice's own depth range

			const float quantum = kNearExtendQuantum * radius;
			float wanted = std::ceil((casterReach + kNearPadding) / quantum) * quantum;
			wanted = std::min(wanted, kMaxNearExtendFactor * radius);
			if (wanted > m_NearExtend[c] || wanted < m_NearExtend[c] - quantum)
				m_NearExtend[c] = wanted; // grow immediately, shrink only past one step
			const float nearExtend = m_NearExtend[c];

			// depth window: 2*radius in front of the eye (the slice), nearExtend behind it (casters)
			const float depthRange = 2.0f * radius + nearExtend;
			glm::mat4 lightProj = glm::ortho(-radius, radius, -radius, radius, -2.0f * radius, nearExtend);
			glm::mat4 lightViewProj = lightProj * lightView;

			m_LightViewProj[c] = lightViewProj;
			m_TexelWorldSize[c] = texelWorld;

			// extract the 6 light frustum planes (same convention as CullingPass)
			const glm::mat4& m = lightViewProj;
			glm::vec4* planes = &m_Planes[c * 6];
			planes[0] = glm::vec4(m[0][3] + m[0][0], m[1][3] + m[1][0], m[2][3] + m[2][0], m[3][3] + m[3][0]);
			planes[1] = glm::vec4(m[0][3] - m[0][0], m[1][3] - m[1][0], m[2][3] - m[2][0], m[3][3] - m[3][0]);
			planes[2] = glm::vec4(m[0][3] + m[0][1], m[1][3] + m[1][1], m[2][3] + m[2][1], m[3][3] + m[3][1]);
			planes[3] = glm::vec4(m[0][3] - m[0][1], m[1][3] - m[1][1], m[2][3] - m[2][1], m[3][3] - m[3][1]);
			planes[4] = glm::vec4(m[0][3] + m[0][2], m[1][3] + m[1][2], m[2][3] + m[2][2], m[3][3] + m[3][2]);
			planes[5] = glm::vec4(m[0][3] - m[0][2], m[1][3] - m[1][2], m[2][3] - m[2][2], m[3][3] - m[3][2]);
			for (int p = 0; p < 6; ++p)
			{
				const float len = glm::length(glm::vec3(planes[p]));
				if (len > 0.0f)
					planes[p] /= len;
			}

			// scene data
			sceneData.lightViewProj[c] = lightViewProj;
			sceneData.cascadeSplits[c] = sliceFar;
			sceneData.cascadeTexelWorld[c] = texelWorld;
			sceneData.cascadeDepthRange[c] = depthRange;

			prevSplit = sliceFar;
		}

		sceneData.shadowParams = glm::vec4(kDepthBiasWorld, kLightSize, kNormalOffset, static_cast<float>(kCascadeCount));
	}

	bool ShadowPass::EnsureBuffers(uint32_t frameIndex, uint32_t objectCount)
	{
		const size_t commandBytes = static_cast<size_t>(objectCount + 64) * sizeof(DrawIndexedIndirectCommand);

		bool insufficient = false;
		for (uint32_t c = 0; c < kCascadeCount && !insufficient; ++c)
		{
			insufficient = !m_CommandBuffers[frameIndex][c] || m_CommandBuffers[frameIndex][c]->GetSize() < commandBytes;
		}
		if (!insufficient)
			return false;

		// only this slot's buffers are recreated; this slot's previous submission already
		// completed (its fence was waited in BeginFrame), so the old buffers are unused
		for (uint32_t c = 0; c < kCascadeCount; ++c)
		{
			m_CommandBuffers[frameIndex][c] = m_Context->device->CreateBuffer(
				commandBytes, BufferUsage::StorageBuffer | BufferUsage::IndirectBuffer, false);
		}
		return true;
	}

	void ShadowPass::UpdateDescriptorSets(uint32_t frameIndex, CullingPass& culling)
	{
		for (uint32_t c = 0; c < kCascadeCount; ++c)
		{
			auto& set = m_CullSets[frameIndex][c];
			set->UpdateBuffer(0, *culling.GetAabbBuffer());
			set->UpdateBuffer(1, *culling.GetInputCommandsBuffer());
			set->UpdateBuffer(2, *m_CommandBuffers[frameIndex][c]);
			set->UpdateBuffer(3, *m_CounterBuffers[frameIndex][c]);
		}
	}

	void ShadowPass::Execute(RHICommandList* cmd, uint32_t cascadeIndex, CullingPass& culling, RHIImage* shadowMap)
	{
		const uint32_t frameIndex = m_Context->device->GetCurrentFrameIndex();
		const uint32_t objectCount = culling.GetObjectCount();

		if (cascadeIndex == 0)
		{
			// read the previous frame's caster counts (this slot's fence already signaled)
			for (uint32_t c = 0; c < kCascadeCount; ++c)
				m_CounterBuffers[frameIndex][c]->ReadData(&m_LastVisibleCounts[c], sizeof(uint32_t), 0);
		}

		if (objectCount > 0)
		{
			EnsureBuffers(frameIndex, objectCount);

			// refresh this slot's bindings once per frame, in cascade 0: the culling pass may
			// have recreated the shared AABB/command buffers, and no shadow cull set has been
			// bound in this command buffer yet (updating a bound set invalidates recording)
			if (cascadeIndex == 0)
				UpdateDescriptorSets(frameIndex, culling);

			const uint32_t zero = 0;
			cmd->FillBuffer(*m_CounterBuffers[frameIndex][cascadeIndex], &zero, sizeof(zero), 0);
			{
				// the counter clear is a transfer write; make it visible to the compute atomics
				MemoryBarrier counterBarrier{};
				counterBarrier.srcStageMask = PipelineStage::Transfer;
				counterBarrier.srcAccessMask = AccessFlags::TransferWrite;
				counterBarrier.dstStageMask = PipelineStage::ComputeShader;
				counterBarrier.dstAccessMask = AccessFlags::ShaderRead | AccessFlags::ShaderWrite;
				cmd->PipelineBarrier(counterBarrier);
			}

			ShadowCullPushConstants push{};
			std::memcpy(push.planes, &m_Planes[cascadeIndex * 6], sizeof(push.planes));
			push.objectCount = objectCount;

			cmd->BindPipeline(*m_CullPipeline);
			cmd->BindDescriptorSet(*m_CullPipelineLayout, *m_CullSets[frameIndex][cascadeIndex], 0, PipelineBindPoint::Compute);
			cmd->BindDescriptorSet(*m_CullPipelineLayout, *m_Context->sceneDataDescriptorSet[frameIndex], 1, PipelineBindPoint::Compute);
			cmd->PushConstants(*m_CullPipelineLayout, ShaderStage::Compute, &push, sizeof(push), 0);
			cmd->Dispatch((objectCount + kCullGroupSize - 1) / kCullGroupSize, 1, 1);

			MemoryBarrier barrier{};
			barrier.srcStageMask = PipelineStage::ComputeShader;
			barrier.srcAccessMask = AccessFlags::ShaderWrite;
			barrier.dstStageMask = PipelineStage::DrawIndirect;
			barrier.dstAccessMask = AccessFlags::IndirectCommandRead;
			cmd->PipelineBarrier(barrier);
		}

		RHIClearValue depthClear{};
		depthClear.isDepth = true;
		depthClear.depthStencil.depth = 1.0f;

		std::vector<RHIRenderingAttachment> attachments = {
			{ shadowMap, ImageLayout::DepthStencilAttachment, AttachmentLoadOp::Clear, AttachmentStoreOp::Store, &depthClear, true, cascadeIndex }
		};

		cmd->BeginRendering(attachments);
		cmd->SetScissor(0, 0, kShadowResolution, kShadowResolution);
		cmd->SetViewport(0.0f, 0.0f, static_cast<float>(kShadowResolution), static_cast<float>(kShadowResolution), 0.0f, 1.0f);

		if (objectCount > 0)
		{
			ShadowRasterPushConstants rasterPush{};
			rasterPush.lightViewProj = m_LightViewProj[cascadeIndex];
			rasterPush.texelWorld = m_TexelWorldSize[cascadeIndex];
			rasterPush.normalOffset = kNormalOffset;

			cmd->BindPipeline(*m_RasterPipeline);
			cmd->BindVertexBuffer(*Mesh::GetGlobalVertexBuffer(), 0, 0);
			cmd->BindIndexBuffer(*Mesh::GetGlobalIndexBuffer(), 0);
			cmd->BindDescriptorSet(*m_RasterPipelineLayout, *m_Context->sceneDataDescriptorSet[frameIndex], 0);
			cmd->PushConstants(*m_RasterPipelineLayout, ShaderStage::Vertex, &rasterPush, sizeof(rasterPush), 0);
			cmd->DrawIndexedIndirectCount(*m_CommandBuffers[frameIndex][cascadeIndex], *m_CounterBuffers[frameIndex][cascadeIndex],
				objectCount, sizeof(DrawIndexedIndirectCommand), 0, 0);
		}

		cmd->EndRendering();
	}
}
