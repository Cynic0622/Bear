#include "bearpch.h"
#include "RenderGraph.h"
#include "RHI/RHIResources.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIDevice.h"

namespace Bear
{
	namespace
	{
		constexpr uint32_t kInvalidPass = 0xFFFFFFFFu;

		// Tracks the last writer of a resource and the readers that consumed the current version.
		struct AccessState
		{
			uint32_t lastWriter = kInvalidPass;
			std::vector<uint32_t> readers;
		};
	}

	void RenderGraph::Setup(RHIDevice* device, uint32_t framesInFlight)
	{
		m_Device = device;
		m_FramesInFlight = framesInFlight > 0 ? framesInFlight : 1;
		m_FrameStamp = 0;
		m_TexturePools.clear();
		m_BufferPools.clear();
	}

	void RenderGraph::Reset()
	{
		m_Textures.clear();
		m_Buffers.clear();
		m_Passes.clear();
		m_ExecutionOrder.clear();
		m_Compiled = false;
	}

	RenderGraph::TextureHandle RenderGraph::ImportTexture(const char* name, RHIImage* image, ImageLayout initialLayout)
	{
		TextureResource res;
		res.name = name ? name : "texture";
		res.image = image;
		res.initialLayout = initialLayout;
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

	RenderGraph::TextureHandle RenderGraph::CreateTexture(const char* name, const RHITextureConfig& config)
	{
		TextureResource res;
		res.name = name ? name : "texture";
		res.initialLayout = ImageLayout::Undefined;
		res.layout = ImageLayout::Undefined;
		res.transient = true;
		res.config = config;
		m_Textures.push_back(std::move(res));
		return TextureHandle{ static_cast<uint32_t>(m_Textures.size() - 1) };
	}

	RenderGraph::BufferHandle RenderGraph::CreateBuffer(const char* name, size_t size, BufferUsage usage, bool cpuAccessible)
	{
		BufferResource res;
		res.name = name ? name : "buffer";
		res.transient = true;
		res.size = size;
		res.usage = usage;
		res.cpuAccessible = cpuAccessible;
		m_Buffers.push_back(std::move(res));
		return BufferHandle{ static_cast<uint32_t>(m_Buffers.size() - 1) };
	}

	RHIImage* RenderGraph::GetTexture(TextureHandle handle) const
	{
		if (handle.id >= m_Textures.size())
			return nullptr;
		return m_Textures[handle.id].image;
	}

	RHIBuffer* RenderGraph::GetBuffer(BufferHandle handle) const
	{
		if (handle.id >= m_Buffers.size())
			return nullptr;
		return m_Buffers[handle.id].buffer;
	}

	void RenderGraph::AllocateTransients()
	{
		if (!m_Device)
			return;

		const uint32_t slot = m_Device->GetCurrentFrameIndex();
		if (m_TexturePools.size() <= slot)
			m_TexturePools.resize(slot + 1);
		if (m_BufferPools.size() <= slot)
			m_BufferPools.resize(slot + 1);
		auto& texturePool = m_TexturePools[slot];
		auto& bufferPool = m_BufferPools[slot];

		++m_FrameStamp;

		// drop pool entries that have not been used for more than framesInFlight compiles,
		// then start a fresh set of live ranges for this frame
		const uint64_t staleBefore = m_FrameStamp > m_FramesInFlight ? m_FrameStamp - m_FramesInFlight : 0;
		auto evictStaleTextures = [&](std::vector<PooledTexture>& pool)
		{
			pool.erase(std::remove_if(pool.begin(), pool.end(),
				[&](const PooledTexture& entry) { return entry.lastUsedStamp < staleBefore; }), pool.end());
			for (auto& entry : pool)
				entry.intervals.clear();
		};
		auto evictStaleBuffers = [&](std::vector<PooledBuffer>& pool)
		{
			pool.erase(std::remove_if(pool.begin(), pool.end(),
				[&](const PooledBuffer& entry) { return entry.lastUsedStamp < staleBefore; }), pool.end());
			for (auto& entry : pool)
				entry.intervals.clear();
		};
		evictStaleTextures(texturePool);
		evictStaleBuffers(bufferPool);

		// live ranges (in execution-order positions) for every resource
		const int32_t unused = -1;
		std::vector<int32_t> textureFirst(m_Textures.size(), unused);
		std::vector<int32_t> textureLast(m_Textures.size(), unused);
		std::vector<int32_t> bufferFirst(m_Buffers.size(), unused);
		std::vector<int32_t> bufferLast(m_Buffers.size(), unused);

		for (uint32_t order = 0; order < m_ExecutionOrder.size(); ++order)
		{
			const auto& pass = m_Passes[m_ExecutionOrder[order]];
			for (const auto& use : pass.textureUses)
			{
				auto& first = textureFirst[use.handle.id];
				if (first == unused)
					first = static_cast<int32_t>(order);
				textureLast[use.handle.id] = static_cast<int32_t>(order);
			}
			for (const auto& use : pass.bufferUses)
			{
				auto& first = bufferFirst[use.handle.id];
				if (first == unused)
					first = static_cast<int32_t>(order);
				bufferLast[use.handle.id] = static_cast<int32_t>(order);
			}
		}

		auto overlaps = [](const std::vector<std::pair<uint32_t, uint32_t>>& intervals, uint32_t first, uint32_t last)
		{
			for (const auto& interval : intervals)
			{
				if (!(last < interval.first || first > interval.second))
					return true;
			}
			return false;
		};

		auto sameTextureConfig = [](const RHITextureConfig& a, const RHITextureConfig& b)
		{
			return a.width == b.width && a.height == b.height && a.format == b.format
				&& a.usage == b.usage && a.mipLevels == b.mipLevels;
		};

		// alias resources with identical descriptions whose life ranges do not overlap
		for (uint32_t i = 0; i < m_Textures.size(); ++i)
		{
			auto& res = m_Textures[i];
			if (!res.transient || textureFirst[i] == unused)
				continue;

			const uint32_t first = static_cast<uint32_t>(textureFirst[i]);
			const uint32_t last = static_cast<uint32_t>(textureLast[i]);

			PooledTexture* chosen = nullptr;
			for (auto& entry : texturePool)
			{
				if (!sameTextureConfig(entry.config, res.config))
					continue;
				if (!overlaps(entry.intervals, first, last))
				{
					chosen = &entry;
					break;
				}
			}
			if (!chosen)
			{
				PooledTexture entry;
				entry.config = res.config;
				entry.image = m_Device->CreateTexture(res.config);
				texturePool.push_back(std::move(entry));
				chosen = &texturePool.back();
			}

			chosen->intervals.emplace_back(first, last);
			chosen->lastUsedStamp = m_FrameStamp;
			res.image = chosen->image.get();
		}

		for (uint32_t i = 0; i < m_Buffers.size(); ++i)
		{
			auto& res = m_Buffers[i];
			if (!res.transient || bufferFirst[i] == unused)
				continue;

			const uint32_t first = static_cast<uint32_t>(bufferFirst[i]);
			const uint32_t last = static_cast<uint32_t>(bufferLast[i]);

			PooledBuffer* chosen = nullptr;
			for (auto& entry : bufferPool)
			{
				if (entry.size != res.size || entry.usage != res.usage || entry.cpuAccessible != res.cpuAccessible)
					continue;
				if (!overlaps(entry.intervals, first, last))
				{
					chosen = &entry;
					break;
				}
			}
			if (!chosen)
			{
				PooledBuffer entry;
				entry.size = res.size;
				entry.usage = res.usage;
				entry.cpuAccessible = res.cpuAccessible;
				entry.buffer = m_Device->CreateBuffer(res.size, res.usage, res.cpuAccessible);
				bufferPool.push_back(std::move(entry));
				chosen = &bufferPool.back();
			}

			chosen->intervals.emplace_back(first, last);
			chosen->lastUsedStamp = m_FrameStamp;
			res.buffer = chosen->buffer.get();
		}
	}

	void RenderGraph::AddPass(const char* name, std::function<void(PassBuilder&)>&& setup, std::function<void(RHICommandList&)>&& execute,
		bool enabled, bool sideEffects)
	{
		PassNode pass;
		pass.name = name ? name : "pass";
		pass.enabled = enabled;
		pass.sideEffects = sideEffects;
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

	void RenderGraph::ComputePassLiveness()
	{
		for (auto& pass : m_Passes)
			pass.culled = !pass.enabled;

		auto textureOutputConsumed = [&](uint32_t passIndex, const std::vector<TextureUseRecord>& writes)
		{
			for (const auto& write : writes)
			{
				if (write.kind != UseKind::Write)
					continue;
				for (uint32_t q = 0; q < m_Passes.size(); ++q)
				{
					if (q == passIndex || m_Passes[q].culled)
						continue;
					for (const auto& use : m_Passes[q].textureUses)
					{
						if (use.handle == write.handle && use.kind == UseKind::Read)
							return true;
					}
				}
			}
			return false;
		};

		auto bufferOutputConsumed = [&](uint32_t passIndex, const std::vector<BufferUseRecord>& writes)
		{
			for (const auto& write : writes)
			{
				if (write.kind != UseKind::Write)
					continue;
				for (uint32_t q = 0; q < m_Passes.size(); ++q)
				{
					if (q == passIndex || m_Passes[q].culled)
						continue;
					for (const auto& use : m_Passes[q].bufferUses)
					{
						if (use.handle == write.handle && use.kind == UseKind::Read)
							return true;
					}
				}
			}
			return false;
		};

		// a pass is live when it has side effects or when a live pass consumes one of its
		// outputs (read); iterate to a fixed point
		bool changed = true;
		while (changed)
		{
			changed = false;
			for (uint32_t p = 0; p < m_Passes.size(); ++p)
			{
				auto& pass = m_Passes[p];
				if (pass.culled || pass.sideEffects)
					continue;

				const bool consumed = textureOutputConsumed(p, pass.textureUses) || bufferOutputConsumed(p, pass.bufferUses);
				if (!consumed)
				{
					pass.culled = true;
					changed = true;
				}
			}
		}
	}

	void RenderGraph::BuildDependencyGraph()
	{
		const size_t passCount = m_Passes.size();

		m_ExecutionOrder.clear();
		m_ExecutionOrder.reserve(passCount);

		std::vector<std::vector<uint32_t>> adjacency(passCount);
		for (auto& pass : m_Passes)
			pass.dependencies.clear();

		auto addEdge = [&](uint32_t from, uint32_t to, std::string label)
		{
			if (from == to || from == kInvalidPass || to == kInvalidPass)
				return;

			auto& dependencies = m_Passes[to].dependencies;
			for (const auto& dep : dependencies)
			{
				if (dep.from == from && dep.label == label)
					return;
			}
			dependencies.push_back({ from, std::move(label) });

			auto& targets = adjacency[from];
			if (std::find(targets.begin(), targets.end(), to) == targets.end())
				targets.push_back(to);
		};

		// Hazards are normalized to declaration order; the only exception is a read that
		// appears before the first writer of an Undefined-imported texture: there is no
		// version to read yet, so the producer is scheduled first.
		std::vector<AccessState> textureStates(m_Textures.size());
		std::vector<AccessState> bufferStates(m_Buffers.size());

		auto analyzeAccess = [&](uint32_t passIndex, AccessState& state, bool isWrite, bool undefinedImport, const std::string& label)
		{
			if (isWrite)
			{
				if (state.lastWriter != kInvalidPass)
					addEdge(state.lastWriter, passIndex, "waw: " + label);

				for (uint32_t reader : state.readers)
				{
					if (reader == passIndex)
						continue;
					if (state.lastWriter == kInvalidPass && undefinedImport)
						addEdge(passIndex, reader, "raw: " + label); // reorder the producer before its consumer
					else
						addEdge(reader, passIndex, "war: " + label);
				}
				state.lastWriter = passIndex;
			}
			else
			{
				if (state.lastWriter != kInvalidPass)
					addEdge(state.lastWriter, passIndex, "raw: " + label);
				if (std::find(state.readers.begin(), state.readers.end(), passIndex) == state.readers.end())
					state.readers.push_back(passIndex);
			}
		};

		for (uint32_t passIndex = 0; passIndex < passCount; ++passIndex)
		{
			const auto& pass = m_Passes[passIndex];
			if (pass.culled)
				continue;
			for (const auto& record : pass.textureUses)
			{
				const auto& res = m_Textures[record.handle.id];
				analyzeAccess(passIndex, textureStates[record.handle.id], record.kind == UseKind::Write,
					res.initialLayout == ImageLayout::Undefined, "texture " + res.name);
			}
			for (const auto& record : pass.bufferUses)
			{
				const auto& res = m_Buffers[record.handle.id];
				analyzeAccess(passIndex, bufferStates[record.handle.id], record.kind == UseKind::Write,
					false, "buffer " + res.name);
			}
		}

		// stable topological sort: ties keep declaration order
		std::vector<uint32_t> inDegree(passCount, 0);
		for (const auto& targets : adjacency)
		{
			for (uint32_t to : targets)
				++inDegree[to];
		}

		std::vector<bool> emitted(passCount, false);
		size_t liveCount = 0;
		for (uint32_t i = 0; i < passCount; ++i)
		{
			if (m_Passes[i].culled)
				emitted[i] = true;
			else
				++liveCount;
		}

		bool cyclic = false;
		for (size_t count = 0; count < liveCount; ++count)
		{
			uint32_t next = kInvalidPass;
			for (uint32_t i = 0; i < passCount; ++i)
			{
				if (!emitted[i] && inDegree[i] == 0)
				{
					next = i;
					break;
				}
			}
			if (next == kInvalidPass)
			{
				cyclic = true;
				break;
			}

			emitted[next] = true;
			m_ExecutionOrder.push_back(next);
			for (uint32_t to : adjacency[next])
				--inDegree[to];
		}

		if (cyclic)
		{
			std::string remaining;
			for (uint32_t i = 0; i < passCount; ++i)
			{
				if (!emitted[i])
					remaining += (remaining.empty() ? "" : ", ") + m_Passes[i].name;
			}
			BEAR_CORE_ERROR("RenderGraph: dependency cycle detected ({}), falling back to declaration order", remaining);

			m_ExecutionOrder.clear();
			for (uint32_t i = 0; i < passCount; ++i)
			{
				if (!m_Passes[i].culled)
					m_ExecutionOrder.push_back(i);
			}
		}
	}

	void RenderGraph::Compile()
	{
		BEAR_CORE_ASSERT(!m_Compiled, "graph already compiled this frame");

		ComputePassLiveness();
		BuildDependencyGraph();
		AllocateTransients();

		for (uint32_t passIndex : m_ExecutionOrder)
		{
			auto& pass = m_Passes[passIndex];
			pass.barriers = {};

			for (const auto& record : pass.textureUses)
			{
				auto& res = m_Textures[record.handle.id];
				const auto& use = record.use;

				// barrier only when the state actually changes or a hazard exists:
				//  - layout transition (incl. the first use of an Undefined import)
				//  - RAW/WAW: the previous use was a write
				//  - WAR: this use writes while an earlier use read
				// pure read-after-read in the same layout needs no barrier
				const bool firstUse = !res.everUsed;
				const bool layoutChanged = res.layout != use.layout;
				const bool writeHazard = !firstUse && res.lastWrite;
				const bool antiHazard = !firstUse && !res.lastWrite && record.kind == UseKind::Write;
				const bool needsBarrier = firstUse ? layoutChanged : (layoutChanged || writeHazard || antiHazard);

				// merge with an existing barrier for the same texture within this pass: once a
				// barrier was emitted it must cover every later use in the pass (widen dst scope)
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
					existing->dstStageMask = existing->dstStageMask | use.stage;
					existing->dstAccessMask = existing->dstAccessMask | use.access;
					existing->newLayout = use.layout;
				}
				else if (needsBarrier)
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

		for (uint32_t passIndex : m_ExecutionOrder)
		{
			auto& pass = m_Passes[passIndex];
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

		auto dumpPass = [&](const PassNode& pass)
		{
			out += "  Pass '" + pass.name + "'\n";
			for (const auto& dep : pass.dependencies)
			{
				out += "    depends on " + m_Passes[dep.from].name + " (" + dep.label + ")\n";
			}
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
		};

		if (m_Compiled)
		{
			out += "  Execution order:";
			for (size_t i = 0; i < m_ExecutionOrder.size(); ++i)
			{
				out += (i == 0 ? " " : " -> ");
				out += m_Passes[m_ExecutionOrder[i]].name;
			}
			out += "\n";

			for (uint32_t passIndex : m_ExecutionOrder)
				dumpPass(m_Passes[passIndex]);
			for (const auto& pass : m_Passes)
			{
				if (pass.culled)
				{
					out += "  Pass '" + pass.name + "' [culled";
					out += pass.enabled ? "]\n" : ", disabled]\n";
				}
			}
		}
		else
		{
			for (const auto& pass : m_Passes)
				dumpPass(pass);
		}
		return out;
	}
}
