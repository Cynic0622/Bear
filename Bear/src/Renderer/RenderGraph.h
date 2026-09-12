#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include "RHI/RHITypes.h"

namespace Bear
{
	class RHICommandList;
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

	// Lightweight frame graph (stage 1+2 of the staged plan):
	//  - resources are imported (no transient allocation yet)
	//  - passes declare reads/writes; the graph generates barriers/layout transitions
	//  - pass execution order = declaration order (validated by Compile)
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

		void Reset();

		TextureHandle ImportTexture(const char* name, RHIImage* image, ImageLayout initialLayout = ImageLayout::Undefined);
		BufferHandle ImportBuffer(const char* name, RHIBuffer* buffer);

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

		void AddPass(const char* name, std::function<void(PassBuilder&)>&& setup, std::function<void(RHICommandList&)>&& execute);

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
			ImageLayout layout = ImageLayout::Undefined;
			PipelineStage lastStage = PipelineStage::TopOfPipe;
			AccessFlags lastAccess = AccessFlags::None;
			bool lastWrite = false;
			bool everUsed = false;
		};
		struct BufferResource
		{
			std::string name;
			RHIBuffer* buffer = nullptr;
			PipelineStage lastStage = PipelineStage::TopOfPipe;
			AccessFlags lastAccess = AccessFlags::None;
			bool lastWrite = false;
			bool everUsed = false;
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

		struct PassNode
		{
			std::string name;
			std::vector<TextureUseRecord> textureUses;
			std::vector<BufferUseRecord> bufferUses;
			std::function<void(RHICommandList&)> execute;
			BarrierPlan barriers;
		};

		void RecordTextureUse(size_t passIndex, TextureHandle handle, UseKind kind, const RGTextureUse& use);
		void RecordBufferUse(size_t passIndex, BufferHandle handle, UseKind kind, const RGBufferUse& use);

		std::vector<TextureResource> m_Textures;
		std::vector<BufferResource> m_Buffers;
		std::vector<PassNode> m_Passes;
		bool m_Compiled = false;
	};
}
