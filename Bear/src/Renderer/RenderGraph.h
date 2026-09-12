#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "RHI/RHITypes.h"

namespace Bear
{
	class RHICommandList;
	class RHIDevice;
	class RHIImage;
	class RHIBuffer;

	// Access declaration for a texture use inside a pass.
	struct RGTextureUse
	{
		PipelineStage stage = PipelineStage::None;
		AccessFlags access = AccessFlags::None;
		ImageLayout layout = ImageLayout::General;
	};

	// Access declaration for a buffer use inside a pass.
	struct RGBufferUse
	{
		PipelineStage stage = PipelineStage::None;
		AccessFlags access = AccessFlags::None;
	};

	// Frame graph:
	//  - resources are either imported (externally owned) or created as transients
	//  - passes declare reads/writes; Compile culls dead passes, derives RAW/WAR/WAW
	//    hazards, computes a stable topological execution order, plans barriers/layout
	//    transitions and aliases transient memory by lifetime
	//  - the whole frame (scene, OIT, UI, present hand-off) is graph-managed
	class RenderGraph
	{
	public:
		struct TextureHandle
		{
			uint32_t id = 0xFFFFFFFFu;
			bool IsValid() const { return id != 0xFFFFFFFFu; }
			bool operator==(const TextureHandle& other) const { return id == other.id; }
		};
		struct BufferHandle
		{
			uint32_t id = 0xFFFFFFFFu;
			bool IsValid() const { return id != 0xFFFFFFFFu; }
			bool operator==(const BufferHandle& other) const { return id == other.id; }
		};

		// device is required for transient resources; framesInFlight sizes the per-slot pools
		void Setup(RHIDevice* device, uint32_t framesInFlight);

		void Reset();

		TextureHandle ImportTexture(const char* name, RHIImage* image, ImageLayout initialLayout = ImageLayout::Undefined);
		BufferHandle ImportBuffer(const char* name, RHIBuffer* buffer);

		// transient resources are allocated by Compile(); the memory is shared (aliased)
		// between resources with identical descriptions and disjoint lifetimes
		TextureHandle CreateTexture(const char* name, const RHITextureConfig& config);
		BufferHandle CreateBuffer(const char* name, size_t size, BufferUsage usage, bool cpuAccessible);

		// valid after Compile(); null when the resource was never used
		RHIImage* GetTexture(TextureHandle handle) const;
		RHIBuffer* GetBuffer(BufferHandle handle) const;

	private:
		enum class UseKind : uint8_t { Read, Write };

	public:
		class PassBuilder
		{
		public:
			void Read(TextureHandle handle, RGTextureUse use) { m_Graph.RecordTextureUse(m_PassIndex, handle, UseKind::Read, use); }
			void Write(TextureHandle handle, RGTextureUse use) { m_Graph.RecordTextureUse(m_PassIndex, handle, UseKind::Write, use); }
			void Read(BufferHandle handle, RGBufferUse use) { m_Graph.RecordBufferUse(m_PassIndex, handle, UseKind::Read, use); }
			void Write(BufferHandle handle, RGBufferUse use) { m_Graph.RecordBufferUse(m_PassIndex, handle, UseKind::Write, use); }

		private:
			friend class RenderGraph;
			PassBuilder(RenderGraph& graph, size_t passIndex) : m_Graph(graph), m_PassIndex(passIndex) {}
			RenderGraph& m_Graph;
			size_t m_PassIndex;
		};

		// enabled=false passes are skipped; sideEffects=true passes always survive culling
		// even when nothing consumes their outputs (e.g. the present transition)
		void AddPass(const char* name, std::function<void(PassBuilder&)>&& setup, std::function<void(RHICommandList&)>&& execute,
			bool enabled = true, bool sideEffects = false);

		// builds the barrier plan; must be called before Execute
		void Compile();
		void Execute(RHICommandList& cmd);

		// human-readable dump of passes, uses and generated barriers (debugging)
		std::string Dump() const;

		struct TextureUseRecord
		{
			TextureHandle handle;
			UseKind kind;
			RGTextureUse use;
		};
		struct BufferUseRecord
		{
			BufferHandle handle;
			UseKind kind;
			RGBufferUse use;
		};

		struct TextureResource
		{
			std::string name;
			RHIImage* image = nullptr;
			ImageLayout initialLayout = ImageLayout::Undefined; // layout the resource enters the frame with (immutable)
			ImageLayout layout = ImageLayout::Undefined;        // tracked layout during Compile
			PipelineStage lastStage = PipelineStage::TopOfPipe;
			AccessFlags lastAccess = AccessFlags::None;
			bool lastWrite = false;
			bool everUsed = false;
			bool transient = false;
			RHITextureConfig config; // transient only
		};
		struct BufferResource
		{
			std::string name;
			RHIBuffer* buffer = nullptr;
			PipelineStage lastStage = PipelineStage::TopOfPipe;
			AccessFlags lastAccess = AccessFlags::None;
			bool lastWrite = false;
			bool everUsed = false;
			bool transient = false;
			size_t size = 0; // transient only
			BufferUsage usage = BufferUsage::None;
			bool cpuAccessible = false;
		};

		struct BarrierPlan
		{
			struct ImageEntry
			{
				TextureHandle handle;
				ImageBarrierDesc barrier;
			};
			std::vector<ImageEntry> imageBarriers;
			MemoryBarrier bufferBarrier;
			bool hasBufferBarrier = false;
		};

		struct DependencyEdge
		{
			uint32_t from = 0xFFFFFFFFu; // pass that must execute first
			std::string label;           // "raw/war/waw: resource" (debugging)
		};

		struct PassNode
		{
			std::string name;
			bool enabled = true;
			bool sideEffects = false;
			bool culled = false;
			std::vector<TextureUseRecord> textureUses;
			std::vector<BufferUseRecord> bufferUses;
			std::vector<DependencyEdge> dependencies;
			std::function<void(RHICommandList&)> execute;
			BarrierPlan barriers;
		};

		void RecordTextureUse(size_t passIndex, TextureHandle handle, UseKind kind, const RGTextureUse& use);
		void RecordBufferUse(size_t passIndex, BufferHandle handle, UseKind kind, const RGBufferUse& use);

		// a pass survives when it has side effects or one of its outputs is consumed by a live pass
		void ComputePassLiveness();

		// derives hazards from the declared uses and fills m_ExecutionOrder (stable topo sort)
		void BuildDependencyGraph();

		// allocates transient resources using lifetime-based aliasing in the per-slot pools
		void AllocateTransients();

		struct PooledTexture
		{
			RHITextureConfig config;
			std::shared_ptr<RHIImage> image;
			std::vector<std::pair<uint32_t, uint32_t>> intervals; // live ranges assigned this frame
			uint64_t lastUsedStamp = 0;
		};
		struct PooledBuffer
		{
			size_t size = 0;
			BufferUsage usage = BufferUsage::None;
			bool cpuAccessible = false;
			std::shared_ptr<RHIBuffer> buffer;
			std::vector<std::pair<uint32_t, uint32_t>> intervals;
			uint64_t lastUsedStamp = 0;
		};

		RHIDevice* m_Device = nullptr;
		uint32_t m_FramesInFlight = 1;
		uint64_t m_FrameStamp = 0;
		std::vector<std::vector<PooledTexture>> m_TexturePools; // per frame slot
		std::vector<std::vector<PooledBuffer>> m_BufferPools;

		std::vector<TextureResource> m_Textures;
		std::vector<BufferResource> m_Buffers;
		std::vector<PassNode> m_Passes;
		std::vector<uint32_t> m_ExecutionOrder; // computed by Compile; index -> pass
		bool m_Compiled = false;
	};
}
