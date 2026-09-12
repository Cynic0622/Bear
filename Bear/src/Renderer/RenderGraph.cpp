#include "bearpch.h"
#include "RenderGraph.h"
#include "RHI/RHIResources.h"
#include "RHI/RHICommandList.h"

namespace Bear
{
	void RenderGraph::Reset()
	{
		m_Textures.clear();
		m_Buffers.clear();
		m_Passes.clear();
		m_Compiled = false;
	}

	RenderGraph::TextureHandle RenderGraph::ImportTexture(const char* name, RHIImage* image, ImageLayout initialLayout)
	{
		TextureResource res;
		res.name = name ? name : "texture";
		res.image = image;
		res.layout = initialLayout;
		m_Textures.push_back(std::move(res));
		return TextureHandle{ static_cast<uint32_t>(m_Textures.size() - 1) };
	}

	RenderGraph::BufferHandle RenderGraph::ImportBuffer(const char* name, RHIBuffer* buffer)
	{
		BufferResource res;
		res.name = name ? name : "buffer";
		res.buffer = buffer;
		m_Buffers.push_back(std::move(res));
		return BufferHandle{ static_cast<uint32_t>(m_Buffers.size() - 1) };
	}

	void RenderGraph::AddPass(const char* name, std::function<void(PassBuilder&)>&& setup, std::function<void(RHICommandList&)>&& execute)
	{
		PassNode pass;
		pass.name = name ? name : "pass";
		pass.execute = std::move(execute);
		m_Passes.push_back(std::move(pass));

		PassBuilder builder(*this, m_Passes.size() - 1);
		if (setup)
			setup(builder);
	}

	void RenderGraph::RecordTextureUse(size_t passIndex, TextureHandle handle, UseKind kind, const RGTextureUse& use)
	{
		BEAR_CORE_ASSERT(passIndex < m_Passes.size(), "invalid pass index");
		BEAR_CORE_ASSERT(handle.id < m_Textures.size(), "invalid texture handle");
		m_Passes[passIndex].textureUses.push_back({ handle, kind, use });
	}

	void RenderGraph::RecordBufferUse(size_t passIndex, BufferHandle handle, UseKind kind, const RGBufferUse& use)
	{
		BEAR_CORE_ASSERT(passIndex < m_Passes.size(), "invalid pass index");
		BEAR_CORE_ASSERT(handle.id < m_Buffers.size(), "invalid buffer handle");
		m_Passes[passIndex].bufferUses.push_back({ handle, kind, use });
	}

	void RenderGraph::Compile()
	{
		BEAR_CORE_ASSERT(!m_Compiled, "graph already compiled this frame");

		for (auto& pass : m_Passes)
		{
			pass.barriers = {};

			for (const auto& record : pass.textureUses)
			{
				auto& res = m_Textures[record.handle.id];
				const auto& use = record.use;

				// no barrier needed for the very first use when the initial layout is undefined
				// (the caller guarantees the resource is entered from an undefined state)
				const bool firstUseUndefined = !res.everUsed && res.layout == ImageLayout::Undefined;
				const bool firstUse = !res.everUsed;

				if (!firstUseUndefined && !firstUse)
				{
					// merge with an existing barrier for the same texture within this pass
					ImageBarrierDesc* existing = nullptr;
					for (auto& entry : pass.barriers.imageBarriers)
					{
						if (entry.handle == record.handle)
						{
							existing = &entry.barrier;
							break;
						}
					}

					if (existing)
					{
						existing->dstStageMask = use.stage;
						existing->dstAccessMask = use.access;
						existing->newLayout = use.layout;
					}
					else
					{
						ImageBarrierDesc barrier;
						barrier.srcStageMask = res.lastStage;
						barrier.srcAccessMask = res.lastAccess;
						barrier.dstStageMask = use.stage;
						barrier.dstAccessMask = use.access;
						barrier.oldLayout = res.layout;
						barrier.newLayout = use.layout;
						barrier.baseMipLevel = 0;
						barrier.levelCount = res.image ? res.image->GetMipLevels() : 1;
						pass.barriers.imageBarriers.push_back({ record.handle, barrier });
					}
				}

				res.layout = use.layout;
				res.lastStage = use.stage;
				res.lastAccess = use.access;
				res.lastWrite = (record.kind == UseKind::Write);
				res.everUsed = true;
			}

			for (const auto& record : pass.bufferUses)
			{
				auto& res = m_Buffers[record.handle.id];
				const auto& use = record.use;

				// buffers have no layouts; only writes create hazards
				const bool needBarrier = res.everUsed && (res.lastWrite || record.kind == UseKind::Write);
				if (needBarrier)
				{
					auto& barrier = pass.barriers.bufferBarrier;
					if (!pass.barriers.hasBufferBarrier)
					{
						barrier.srcStageMask = res.lastStage;
						barrier.srcAccessMask = res.lastAccess;
						barrier.dstStageMask = use.stage;
						barrier.dstAccessMask = use.access;
						pass.barriers.hasBufferBarrier = true;
					}
					else
					{
						barrier.srcStageMask = barrier.srcStageMask | res.lastStage;
						barrier.srcAccessMask = barrier.srcAccessMask | res.lastAccess;
						barrier.dstStageMask = barrier.dstStageMask | use.stage;
						barrier.dstAccessMask = barrier.dstAccessMask | use.access;
					}
				}

				res.lastStage = use.stage;
				res.lastAccess = use.access;
				res.lastWrite = (record.kind == UseKind::Write);
				res.everUsed = true;
			}
		}

		m_Compiled = true;
	}

	void RenderGraph::Execute(RHICommandList& cmd)
	{
		BEAR_CORE_ASSERT(m_Compiled, "Compile() must be called before Execute()");

		for (auto& pass : m_Passes)
		{
			for (auto& entry : pass.barriers.imageBarriers)
			{
				if (m_Textures[entry.handle.id].image)
					cmd.ImageBarrier(*m_Textures[entry.handle.id].image, entry.barrier);
			}
			if (pass.barriers.hasBufferBarrier)
				cmd.PipelineBarrier(pass.barriers.bufferBarrier);

			if (pass.execute)
				pass.execute(cmd);
		}
	}

	std::string RenderGraph::Dump() const
	{
		std::string out = "RenderGraph\n";
		for (const auto& pass : m_Passes)
		{
			out += "  Pass '" + pass.name + "'\n";
			for (const auto& record : pass.textureUses)
			{
				out += std::string("    ") + (record.kind == UseKind::Read ? "R" : "W") +
					" texture " + m_Textures[record.handle.id].name +
					" (layout " + std::to_string(static_cast<int>(record.use.layout)) + ")\n";
			}
			for (const auto& record : pass.bufferUses)
			{
				out += std::string("    ") + (record.kind == UseKind::Read ? "R" : "W") +
					" buffer " + m_Buffers[record.handle.id].name + "\n";
			}
			for (const auto& entry : pass.barriers.imageBarriers)
			{
				out += "    barrier: image " + m_Textures[entry.handle.id].name +
					" layout " + std::to_string(static_cast<int>(entry.barrier.oldLayout)) +
					" -> " + std::to_string(static_cast<int>(entry.barrier.newLayout)) + "\n";
			}
			if (pass.barriers.hasBufferBarrier)
				out += "    barrier: global memory\n";
		}
		return out;
	}
}
