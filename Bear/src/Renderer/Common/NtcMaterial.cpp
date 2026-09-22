#include "bearpch.h"
#include "NtcMaterial.h"
#include "Texture.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace Bear
{
	// -----------------------------------------------------------------------------------------
	// Background compression
	//
	// NTC compression is a long CUDA job (100k training steps by default), so it runs on a worker
	// thread instead of blocking the frame. The worker only ever touches this struct; the material
	// itself stays owned by the main thread, which is why the signals are shared pointers - the job
	// must remain safe to touch even after the material that started it is gone - which is why the
	// worker never dereferences the material, only this struct.
	// -----------------------------------------------------------------------------------------
	struct NtcCompressionJob
	{
		std::string materialName;
		std::string filePath;
		NtcChannelPlan plan;
		std::unordered_map<int, std::shared_ptr<Texture>> textures; // keeps the source pixels alive
		int width = 1;
		int height = 1;
		int mips = 1;

		std::vector<uint8_t> compressedData;
		bool ok = false;

		std::shared_ptr<std::atomic<bool>> cancelled;
		std::shared_ptr<std::atomic<bool>> finished;
	};

	namespace
	{
		// Compression settings; they are part of the cache file fingerprint.
		constexpr float kNtcExpectedBitsPerPixel = 4.0f;
		constexpr int   kNtcNetworkVersion = NTC_NETWORK_LARGE;
		constexpr int   kNtcTrainingSteps = 100000;

		const Texture* ResolveTexture(const std::unordered_map<int, std::shared_ptr<Texture>>& textures, int textureIndex)
		{
			if (textureIndex < 0)
				return nullptr;
			const auto it = textures.find(textureIndex);
			return it == textures.end() ? nullptr : it->second.get();
		}

		// Failures here are recoverable (bad cache file, out of range args) and BEAR_CORE_ERROR
		// ends with __debugbreak(), so log without interrupting.
		void LogNtcFailure(const char* what, ntc::Status status)
		{
			::Bear::Log::GetCoreLogger()->error("NTC: {} failed, code = {} : {}",
				what, ntc::StatusToString(status), ntc::GetLastErrorMessage());
		}

		// Color space of a semantic channel: base color/emissive are sRGB data, the rest linear.
		ntc::ColorSpace RoleColorSpace(NtcChannelRole role)
		{
			switch (role)
			{
			case NtcChannelRole::BaseColor:
			case NtcChannelRole::Emissive:
				return ntc::ColorSpace::sRGB;
			default:
				return ntc::ColorSpace::Linear;
			}
		}

		// Color space of a texture set channel (looks up which semantic owns it).
		ntc::ColorSpace ChannelColorSpace(const NtcChannelPlan& plan, int channel)
		{
			for (int role = 0; role < kNtcChannelRoleCount; ++role)
				for (int component = 0; component < kNtcRoleMaxComponents; ++component)
					if (plan.sourceChannel[role][component] == channel)
						return RoleColorSpace(static_cast<NtcChannelRole>(role));
			return ntc::ColorSpace::Linear;
		}

		// The set follows the largest input (NTC resamples smaller images up) and uses the full
		// mip chain.
		void ComputeTextureSetSize(const NtcChannelPlan& plan,
			const std::unordered_map<int, std::shared_ptr<Texture>>& textures,
			int& outWidth, int& outHeight, int& outMips)
		{
			uint32_t width = 1, height = 1;
			for (const NtcChannelWrite& write : plan.writes)
			{
				const Texture* texture = ResolveTexture(textures, write.textureIndex);
				if (!texture)
					continue;
				width = (std::max)(width, texture->GetWidth());
				height = (std::max)(height, texture->GetHeight());
			}

			const uint32_t smallest = (std::min)(width, height);
			int mips = smallest > 1
				? static_cast<int>(std::floor(std::log2(static_cast<double>(smallest)))) + 1
				: 1;

			outWidth = static_cast<int>(width);
			outHeight = static_cast<int>(height);
			outMips = (std::min)(mips, NTC_MAX_MIPS);
		}

#ifdef BEAR_DEBUG
		void LogTextureSetContents(ntc::TextureSetWrapper& textureSet, const char* stage)
		{
			if (!textureSet)
				return;
			for (int index = 0; index < textureSet->GetTextureCount(); ++index)
			{
				ntc::ITextureMetadata* textureMetadata = textureSet->GetTexture(index);
				if (!textureMetadata)
					continue;
				const char* name = textureMetadata->GetName();
				BEAR_CORE_INFO("NTC [{}] texture[{}] '{}': channels {}..{}, block compression {}, RGB {}, alpha {}",
					stage, index, name ? name : "?",
					textureMetadata->GetFirstChannel(),
					textureMetadata->GetFirstChannel() + textureMetadata->GetNumChannels() - 1,
					ntc::BlockCompressedFormatToString(textureMetadata->GetBlockCompressedFormat()),
					ntc::ColorSpaceToString(textureMetadata->GetRgbColorSpace()),
					ntc::ColorSpaceToString(textureMetadata->GetAlphaColorSpace()));
			}
		}
#endif

		// --- job queue: a single worker keeps CUDA memory bounded and jobs ordered ---------------
		std::mutex g_QueueMutex;
		std::condition_variable g_QueueCv;
		std::deque<std::shared_ptr<NtcCompressionJob>> g_Queue;
		bool g_QueueStopping = false;
		std::atomic<bool> g_ShuttingDown{ false };

		void MarkJobFinished(const std::shared_ptr<NtcCompressionJob>& job)
		{
			job->finished->store(true);
		}

		// Runs on the worker thread. Writes only into 'job' - never into the material.
		void RunCompressionJob(const std::shared_ptr<NtcCompressionJob>& job)
		{
			if (job->cancelled->load())
				return;

			const NtcChannelPlan& plan = job->plan;
			const std::unordered_map<int, std::shared_ptr<Texture>>& textures = job->textures;

			// The worker owns this context: it is created and destroyed on the same thread, and the
			// main thread never touches it (the material creates its own for the upload step).
			ntc::IContext* context = nullptr;
			ntc::Status ntcStatus = ntc::CreateContext(&context, ntc::ContextParameters{});
			if (ntcStatus != ntc::Status::Ok)
			{
				LogNtcFailure("CreateContext", ntcStatus);
				return;
			}
			struct ContextGuard
			{
				ntc::IContext* context;
				~ContextGuard() { if (context) ntc::DestroyContext(context); }
			} contextGuard{ context };

			ntc::TextureSetWrapper textureSet(context);

			ntc::TextureSetDesc textureSetDesc;
			textureSetDesc.channels = plan.totalChannels;
			textureSetDesc.width = job->width;
			textureSetDesc.height = job->height;
			// Full mip chain: compression, file and runtime LOD selection all become multi-scale.
			// Only mip0 is written here; GenerateMips() derives the rest by resampling.
			textureSetDesc.mips = job->mips;

			ntc::TextureSetFeatures features;
			ntcStatus = context->CreateTextureSet(textureSetDesc, features, textureSet.ptr());
			if (ntcStatus != ntc::Status::Ok) { LogNtcFailure("CreateTextureSet", ntcStatus); return; }

			// Target bit rate and network size.
			float actualBitsPerPixel = 0.f;
			ntc::LatentShape latentShape;
			ntcStatus = ntc::PickLatentShape(kNtcExpectedBitsPerPixel, kNtcNetworkVersion, actualBitsPerPixel, latentShape);
			if (ntcStatus != ntc::Status::Ok) { LogNtcFailure("PickLatentShape", ntcStatus); return; }
			ntcStatus = textureSet->SetLatentShape(latentShape, kNtcNetworkVersion);
			if (ntcStatus != ntc::Status::Ok) { LogNtcFailure("SetLatentShape", ntcStatus); return; }

			BEAR_CORE_INFO("NTC: compressing '{}' ({}x{}, {} channels, {} mips, target {:.2f} bpp -> {:.2f}) on a background thread.",
				job->materialName, job->width, job->height, plan.totalChannels, job->mips,
				kNtcExpectedBitsPerPixel, actualBitsPerPixel);

			const std::vector<uint8_t> placeholderPixel{ 0, 0, 0, 255 };
			for (const NtcChannelWrite& write : plan.writes)
			{
				const Texture* texture = ResolveTexture(textures, write.textureIndex);

				ntc::WriteChannelsParameters writeParams;
				writeParams.mipLevel = 0;
				writeParams.firstChannel = write.firstChannel;
				writeParams.numChannels = write.numChannels;
				writeParams.addressSpace = ntc::AddressSpace::Host;
				writeParams.channelFormat = ntc::ChannelFormat::UNORM8;
				writeParams.pixelStride = 4; // Texture::GetImageData() is always RGBA padded
				if (texture)
				{
					const std::vector<unsigned char>& pixels = texture->GetImageData();
					writeParams.pData = pixels.data() + write.localChannel;
					writeParams.width = static_cast<int>(texture->GetWidth());
					writeParams.height = static_cast<int>(texture->GetHeight());
					writeParams.rowPitch = static_cast<size_t>(texture->GetWidth()) * 4;
				}
				else
				{
					writeParams.pData = placeholderPixel.data();
					writeParams.width = 1;
					writeParams.height = 1;
					writeParams.rowPitch = 4;
				}

				ntc::ColorSpace colorSpaces[kNtcRoleMaxComponents + 1];
				for (int i = 0; i < write.numChannels; ++i)
					colorSpaces[i] = ChannelColorSpace(plan, write.firstChannel + i);
				writeParams.srcColorSpaces = colorSpaces;
				writeParams.dstColorSpaces = colorSpaces;

				ntcStatus = textureSet->WriteChannels(writeParams);
				if (ntcStatus != ntc::Status::Ok) { LogNtcFailure("WriteChannels", ntcStatus); return; }
			}

			// Register metadata per source texture (contiguous ranges of one image merge).
			{
				struct TextureRange { int textureIndex; int firstChannel; int numChannels; };
				std::vector<TextureRange> ranges;
				for (const NtcChannelWrite& write : plan.writes)
				{
					if (write.textureIndex < 0)
						continue;
					if (!ranges.empty() && ranges.back().textureIndex == write.textureIndex &&
						ranges.back().firstChannel + ranges.back().numChannels == write.firstChannel)
						ranges.back().numChannels += write.numChannels;
					else
						ranges.push_back({ write.textureIndex, write.firstChannel, write.numChannels });
				}

				for (const TextureRange& range : ranges)
				{
					ntc::ITextureMetadata* metadata = textureSet->AddTexture();
					if (!metadata)
						continue;
					const Texture* texture = ResolveTexture(textures, range.textureIndex);
					if (texture)
						metadata->SetName(texture->GetName().c_str());
					metadata->SetChannels(range.firstChannel, range.numChannels);
					metadata->SetRgbColorSpace(ChannelColorSpace(plan, range.firstChannel));
					metadata->SetAlphaColorSpace(range.numChannels > 3
						? ChannelColorSpace(plan, range.firstChannel + 3)
						: ntc::ColorSpace::Linear);
				}
			}

			// Only mip0 was written; this downsamples the rest of the input mip chain.
			ntcStatus = textureSet->GenerateMips();
			if (ntcStatus != ntc::Status::Ok) { LogNtcFailure("GenerateMips", ntcStatus); return; }

			ntc::CompressionSettings compSettings;
			compSettings.trainingSteps = kNtcTrainingSteps;
			ntcStatus = textureSet->BeginCompression(compSettings);
			if (ntcStatus != ntc::Status::Ok) { LogNtcFailure("BeginCompression", ntcStatus); return; }

			ntc::CompressionStats stats;
			do
			{
				if (job->cancelled->load() || g_ShuttingDown.load())
				{
					BEAR_CORE_INFO("NTC: compression of '{}' cancelled.", job->materialName);
					return;
				}

				ntcStatus = textureSet->RunCompressionSteps(&stats);
				if (ntcStatus != ntc::Status::Ok && ntcStatus != ntc::Status::Incomplete)
				{
					LogNtcFailure("RunCompressionSteps", ntcStatus);
					return;
				}
				BEAR_CORE_INFO("NTC '{}': step {}/{}, loss = {:.6f} (PSNR {:.2f} dB)",
					job->materialName, stats.currentStep, compSettings.trainingSteps, stats.loss, ntc::LossToPSNR(stats.loss));
			} while (ntcStatus == ntc::Status::Incomplete);

			ntcStatus = textureSet->FinalizeCompression();
			if (ntcStatus != ntc::Status::Ok) { LogNtcFailure("FinalizeCompression", ntcStatus); return; }

#ifdef BEAR_DEBUG
			LogTextureSetContents(textureSet, "compressed");
#endif

			ntcStatus = textureSet->SaveToFile(job->filePath.c_str());
			if (ntcStatus != ntc::Status::Ok) { LogNtcFailure("SaveToFile", ntcStatus); return; }

			size_t streamSize = static_cast<size_t>(textureSet->GetOutputStreamSize());
			job->compressedData.resize(streamSize);
			ntcStatus = textureSet->SaveToMemory(job->compressedData.data(), &streamSize);
			if (ntcStatus != ntc::Status::Ok) { LogNtcFailure("SaveToMemory", ntcStatus); return; }

			job->ok = true;
		}

		void WorkerLoop()
		{
			for (;;)
			{
				std::shared_ptr<NtcCompressionJob> job;
				{
					std::unique_lock<std::mutex> lock(g_QueueMutex);
					g_QueueCv.wait(lock, [] { return g_QueueStopping || !g_Queue.empty(); });
					if (g_Queue.empty())
						return; // stopping and drained
					job = std::move(g_Queue.front());
					g_Queue.pop_front();
				}

				RunCompressionJob(job);
				MarkJobFinished(job);
			}
		}

		// Started on first use; destroyed (and joined) at process exit.
		struct CompressionWorker
		{
			CompressionWorker() : thread(WorkerLoop) {}
			~CompressionWorker()
			{
				// Wake the running job (it checks this between training steps) and release anything
				// still queued, so no material can be left waiting on a job that will never run.
				g_ShuttingDown.store(true);
				{
					std::lock_guard<std::mutex> lock(g_QueueMutex);
					g_QueueStopping = true;
					for (const auto& job : g_Queue)
					{
						job->cancelled->store(true);
						MarkJobFinished(job);
					}
					g_Queue.clear();
				}
				g_QueueCv.notify_all();
				if (thread.joinable())
					thread.join();
			}
			std::thread thread;
		};

		void EnsureWorkerStarted()
		{
			static CompressionWorker worker;
			(void)worker;
		}
	}

	NtcMaterial::NtcMaterial(RHIDevice& device, const MaterialDescription& desc, const std::unordered_map<int, std::shared_ptr<Texture>>& textures)
		: m_Device(device), m_TextureSet(nullptr)
	{
		m_DescriptorSetLayout = device.CreateDescriptorSetLayout(GetDescriptorSetLayoutBinding());
		m_DescriptorSets = device.CreateDescriptorSet(m_DescriptorSetLayout);
		networkVersion = kNtcNetworkVersion;

		// Fix the channel layout first: both the write plan and the shader-side slot map derive
		// from it.
		BuildChannelPlan(desc, textures);

		// Materials without textures (e.g. the glass materials in TransmissionTest) still need a
		// valid texture set: one 1x1 dummy channel that no slot references.
		if (m_ChannelPlan.writes.empty())
		{
			BEAR_CORE_INFO("NTC: material '{}' has no textures, compressing a constant texture set.", desc.name);
			m_ChannelPlan.writes.push_back(NtcChannelWrite{ -1, 0, 1, 0 });
			m_ChannelPlan.totalChannels = 1;
		}

		if (m_ChannelPlan.totalChannels > NTC_MAX_CHANNELS)
		{
			::Bear::Log::GetCoreLogger()->error("NTC: material '{}' needs {} channels but the limit is {}.",
				desc.name, m_ChannelPlan.totalChannels, NTC_MAX_CHANNELS);
			return;
		}

		int width = 1, height = 1, mips = 1;
		ComputeTextureSetSize(m_ChannelPlan, textures, width, height, mips);
		const std::string filePath = GetCompressedFilePath(desc, textures, mips);

		// Cached result: parsing a few hundred KB is fast, keep it on this thread.
		if (std::filesystem::exists(filePath))
		{
			BEAR_CORE_INFO("NTC: '{}' is already compressed, loading '{}'.", desc.name, filePath);
			if (LoadCompressedFile(filePath))
			{
				UploadTextures();
				m_Uploaded = true;
				return;
			}
			::Bear::Log::GetCoreLogger()->error("NTC: failed to load '{}', recompressing it.", filePath);
		}

		// No cache: compress on the worker thread. IsReady() does the GPU upload on the main thread
		// once the worker is done.
		StartCompression(desc, textures, filePath, width, height, mips);
	}

	NtcMaterial::~NtcMaterial()
	{
		// Ask the worker to abandon this job. No join is needed: the worker only ever touches the
		// job struct (which owns copies of everything it needs), never this material, so the job may
		// safely outlive us - the queue keeps it alive until the worker picks it up and skips it.
		if (m_CompressionJob)
			m_CompressionJob->cancelled->store(true);
	}

	bool NtcMaterial::IsReady()
	{
		if (m_Uploaded)
			return true;
		if (!m_CompressionJob || !m_CompressionJob->finished->load())
			return false;

		if (m_CompressionJob->ok)
		{
			m_CompressedData = std::move(m_CompressionJob->compressedData);
			m_CompressionJob.reset();

			// The material's descriptor set is shared by every frame in flight, and the buffers it
			// points at are about to be replaced: wait for the GPU before swapping them.
			m_Device.WaitIdle();
			UploadTextures();
			m_Uploaded = true;
		}
		else
		{
			// Compression failed (the worker logged why): stay invisible instead of binding a
			// descriptor set without data behind it.
			m_CompressionJob.reset();
		}
		return m_Uploaded;
	}

	void NtcMaterial::StartCompression(const MaterialDescription& desc,
		const std::unordered_map<int, std::shared_ptr<Texture>>& textures,
		const std::string& filePath, int width, int height, int mips)
	{
		auto job = std::make_shared<NtcCompressionJob>();
		job->materialName = desc.name;
		job->filePath = filePath;
		job->plan = m_ChannelPlan;
		job->width = width;
		job->height = height;
		job->mips = mips;
		job->cancelled = std::make_shared<std::atomic<bool>>(false);
		job->finished = std::make_shared<std::atomic<bool>>(false);

		// Keep only the textures the plan uses: the worker needs their CPU pixels, and
		// ResourceManager releases them once it returns.
		for (const NtcChannelWrite& write : m_ChannelPlan.writes)
		{
			if (write.textureIndex < 0)
				continue;
			const auto it = textures.find(write.textureIndex);
			if (it != textures.end() && it->second)
				job->textures.emplace(write.textureIndex, it->second);
		}

		m_CompressionJob = job;

		EnsureWorkerStarted();
		{
			std::lock_guard<std::mutex> lock(g_QueueMutex);
			g_Queue.push_back(job);
		}
		g_QueueCv.notify_one();
	}

	void NtcMaterial::BuildChannelPlan(const MaterialDescription& desc, const std::unordered_map<int, std::shared_ptr<Texture>>& textures)
	{
		std::vector<NtcSourceTexture> sources;
		auto addSource = [&textures, &sources](NtcChannelRole role, int textureIndex)
		{
			const Texture* texture = ResolveTexture(textures, textureIndex);
			if (!texture)
				return;
			NtcSourceTexture source;
			source.role = role;
			source.textureIndex = textureIndex;
			// GetChannels() is the file's own component count (1..4), not the RGBA-padded buffer.
			source.channels = static_cast<uint8_t>(std::clamp<int>(texture->GetChannels(), 1, 4));
			sources.push_back(source);
		};

		addSource(NtcChannelRole::BaseColor, desc.baseColorTextureIndex);
		addSource(NtcChannelRole::Metalness, desc.metallicRoughnessTextureIndex); // metallicRoughness
		addSource(NtcChannelRole::Normal, desc.normalTextureIndex);
		addSource(NtcChannelRole::Occlusion, desc.occlusionTextureIndex);
		addSource(NtcChannelRole::Emissive, desc.emissiveTextureIndex);

		m_ChannelPlan = BuildNtcChannelPlan(sources, desc.isTransparent);

		// Semantics without a texture fall back to the material factors. The shader consumes base
		// color/emissive without decoding, so linear factors are encoded to sRGB storage values.
		for (int c = 0; c < 3; ++c)
			m_SlotConstants.baseColor[c] = NtcEncodeSrgb(desc.baseColorFactor[c]);
		m_SlotConstants.opacity = desc.baseColorFactor.a;
		m_SlotConstants.metalness = desc.metallicFactor;
		m_SlotConstants.roughness = desc.roughnessFactor;
		for (int c = 0; c < 3; ++c)
			m_SlotConstants.emissive[c] = NtcEncodeSrgb(desc.emissiveFactor[c]);
	}

	void NtcMaterial::SetBuffer(NtcMaterialSlot slot, size_t dataSize, const void* data)
	{
		switch (slot)
		{
		case NtcMaterialSlot::Latent:
			m_LatentBuffer = m_Device.CreateBuffer(dataSize, BufferUsage::StorageBuffer, true);
			m_DescriptorSets->UpdateBuffer(NtcMaterialSlot::Latent, *m_LatentBuffer);
			m_LatentBuffer->UploadData(data, dataSize);
			break;
		case NtcMaterialSlot::Weight:
			m_WeightBuffer = m_Device.CreateBuffer(dataSize, BufferUsage::StorageBuffer, true);
			m_DescriptorSets->UpdateBuffer(NtcMaterialSlot::Weight, *m_WeightBuffer);
			m_WeightBuffer->UploadData(data, dataSize);
			break;
		case NtcMaterialSlot::Constants:
			m_ConstantBuffer = m_Device.CreateBuffer(dataSize, BufferUsage::UniformBuffer, true);
			m_DescriptorSets->UpdateBuffer(NtcMaterialSlot::Constants, *m_ConstantBuffer);
			m_ConstantBuffer->UploadData(data, dataSize);
			break;
		default:
			BEAR_CORE_ERROR("Invalid NTC material slot");
			break;
		}
	}

	std::vector<RHIDescriptorSetLayoutBinding> NtcMaterial::s_DescriptorSetLayoutBinding;
	std::vector<RHIDescriptorSetLayoutBinding> NtcMaterial::GetDescriptorSetLayoutBinding()
	{
		if (s_DescriptorSetLayoutBinding.empty())
		{
			s_DescriptorSetLayoutBinding.push_back({ .binding = NtcMaterialSlot::Latent, .descriptorType = DescriptorType::StorageBuffer, .stageFlags = ShaderStage::Fragment });
			s_DescriptorSetLayoutBinding.push_back({ .binding = NtcMaterialSlot::Weight, .descriptorType = DescriptorType::StorageBuffer, .stageFlags = ShaderStage::Fragment });
			s_DescriptorSetLayoutBinding.push_back({ .binding = NtcMaterialSlot::Constants, .descriptorType = DescriptorType::UniformBuffer, .stageFlags = ShaderStage::Fragment });
		}
		return s_DescriptorSetLayoutBinding;
	}

	std::string NtcMaterial::GetCompressedFilePath(const MaterialDescription& desc,
		const std::unordered_map<int, std::shared_ptr<Texture>>& textures, int mipCount) const
	{
		// Name = material name + layout version + source fingerprint: a changed layout, texture or
		// compression setting never reuses an old result, and different materials (e.g. the unnamed
		// ones in Sponza) never share a file. Pixel edits that keep size and channel count are not
		// detected - see the cache invalidation follow-up.
		uint64_t hash = 14695981039346656037ull; // FNV-1a
		auto mix = [&hash](uint64_t value)
		{
			hash ^= value;
			hash *= 1099511628211ull;
		};

		mix(static_cast<uint64_t>(kLayoutVersion));
		mix(static_cast<uint64_t>(kNtcNetworkVersion));
		mix(static_cast<uint64_t>(kNtcExpectedBitsPerPixel * 100.0f));
		mix(static_cast<uint64_t>(kNtcTrainingSteps));
		mix(static_cast<uint64_t>(mipCount)); // the mip chain length changes the content
		mix(desc.isTransparent ? 1ull : 0ull);
		// Material factors become constants in the set, so they matter too (matters most for
		// materials without textures).
		for (int c = 0; c < 3; ++c)
			mix(static_cast<uint64_t>(m_SlotConstants.baseColor[c] * 1000.0f));
		mix(static_cast<uint64_t>(m_SlotConstants.opacity * 1000.0f));
		mix(static_cast<uint64_t>(m_SlotConstants.metalness * 1000.0f));
		mix(static_cast<uint64_t>(m_SlotConstants.roughness * 1000.0f));
		for (int c = 0; c < 3; ++c)
			mix(static_cast<uint64_t>(m_SlotConstants.emissive[c] * 1000.0f));
		for (const NtcChannelWrite& write : m_ChannelPlan.writes)
		{
			mix(static_cast<uint64_t>(write.textureIndex + 1));
			mix(static_cast<uint64_t>(write.localChannel));
			mix(static_cast<uint64_t>(write.numChannels));
		}
		for (int role = 0; role < kNtcChannelRoleCount; ++role)
			for (int component = 0; component < kNtcRoleMaxComponents; ++component)
				mix(static_cast<uint64_t>(m_ChannelPlan.sourceChannel[role][component] + 1));
		for (const NtcChannelWrite& write : m_ChannelPlan.writes)
		{
			const Texture* texture = ResolveTexture(textures, write.textureIndex);
			if (!texture)
				continue;
			mix(static_cast<uint64_t>(texture->GetWidth()));
			mix(static_cast<uint64_t>(texture->GetHeight()));
			mix(static_cast<uint64_t>(texture->GetChannels()));
		}

		// Material names may be empty or contain path separators: sanitize them.
		std::string name = desc.name.empty() ? std::string("unnamed") : desc.name;
		for (char& c : name)
			if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '_')
				c = '_';

		std::ostringstream fileName;
		fileName << "assets/compress/" << name << ".v" << kLayoutVersion << '.'
			<< std::hex << std::setw(8) << std::setfill('0')
			<< static_cast<uint32_t>(hash & 0xFFFFFFFFull) << ".ntc";
		return fileName.str();
	}

	bool NtcMaterial::LoadCompressedFile(const std::string& filePath)
	{
		EnsureContext();
		if (!m_NtcContext)
			return false;

		ntc::FileStreamWrapper inputFile(m_NtcContext);
		ntc::Status ntcStatus = m_NtcContext->OpenFile(filePath.c_str(), false, inputFile.ptr());
		if (ntcStatus != ntc::Status::Ok) { LogNtcFailure("OpenFile", ntcStatus); return false; }

		m_TextureSet = ntc::TextureSetWrapper(m_NtcContext);
		ntc::TextureSetFeatures features;
		ntcStatus = m_NtcContext->CreateCompressedTextureSetFromStream(inputFile, features, m_TextureSet.ptr());
		if (ntcStatus != ntc::Status::Ok) { LogNtcFailure("CreateCompressedTextureSetFromStream", ntcStatus); return false; }

		size_t streamSize = static_cast<size_t>((std::max)(inputFile->Size(), m_TextureSet->GetOutputStreamSize()));
		m_CompressedData.resize(streamSize);
		ntcStatus = m_TextureSet->SaveToMemory(m_CompressedData.data(), &streamSize);
		if (ntcStatus != ntc::Status::Ok) { LogNtcFailure("SaveToMemory", ntcStatus); return false; }

		LogTextureSet("loaded");
		return true;
	}

	void NtcMaterial::EnsureContext()
	{
		if (m_NtcContext)
			return;
		ntc::Status ntcStatus = ntc::CreateContext(&m_NtcContext, ntc::ContextParameters{});
		if (ntcStatus != ntc::Status::Ok)
		{
			LogNtcFailure("CreateContext", ntcStatus);
			m_NtcContext = nullptr;
		}
	}

	void NtcMaterial::UploadTextures()
	{
		EnsureContext();
		if (!m_NtcContext || m_CompressedData.empty())
		{
			::Bear::Log::GetCoreLogger()->error("NTC: no compressed data to upload.");
			return;
		}

		ntc::MemoryStreamWrapper memStream(m_NtcContext);
		ntc::Status ntcStatus = m_NtcContext->OpenReadOnlyMemory(m_CompressedData.data(), m_CompressedData.size(), memStream.ptr());
		if (ntcStatus != ntc::Status::Ok) { LogNtcFailure("OpenReadOnlyMemory", ntcStatus); return; }

		ntc::TextureSetMetadataWrapper textureSetMetadata(m_NtcContext);
		ntcStatus = m_NtcContext->CreateTextureSetMetadataFromStream(memStream, textureSetMetadata.ptr());
		if (ntcStatus != ntc::Status::Ok) { LogNtcFailure("CreateTextureSetMetadataFromStream", ntcStatus); return; }

		// Shuffle the set channels into the semantic slots the shader reads; missing ones get
		// constants.
		std::array<ntc::ShuffleSource, NTC_MAX_CHANNELS> channelMap = BuildShuffleMap();
		ntcStatus = textureSetMetadata->ShuffleInferenceOutputs(channelMap.data());
		if (ntcStatus != ntc::Status::Ok) { LogNtcFailure("ShuffleInferenceOutputs", ntcStatus); return; }

		void const* pWeightData = nullptr;
		size_t weightDataSize = 0;
		size_t convertedSize = 0;
		ntcStatus = textureSetMetadata->GetInferenceWeights(ntc::InferenceWeightType::GenericInt8, &pWeightData, &weightDataSize, &convertedSize);
		if (ntcStatus != ntc::Status::Ok) { LogNtcFailure("GetInferenceWeights", ntcStatus); return; }

		ntc::StreamRange latentRange;
		ntcStatus = textureSetMetadata->GetStreamRangeForLatents(0, textureSetMetadata->GetDesc().mips, latentRange);
		if (ntcStatus != ntc::Status::Ok) { LogNtcFailure("GetStreamRangeForLatents", ntcStatus); return; }

		ntc::InferenceData inferenceData;
		ntcStatus = m_NtcContext->MakeInferenceData(textureSetMetadata, latentRange, ntc::InferenceWeightType::GenericInt8, &inferenceData);
		if (ntcStatus != ntc::Status::Ok) { LogNtcFailure("MakeInferenceData", ntcStatus); return; }

		SetBuffer(NtcMaterialSlot::Constants, sizeof(inferenceData.constants), &inferenceData.constants);

		size_t dataSize = latentRange.size;
		std::vector<uint8_t> latentData(dataSize);
		memStream->Seek(latentRange.offset);
		if (!memStream->Read(latentData.data(), dataSize))
		{
			::Bear::Log::GetCoreLogger()->error("NTC: failed to read the latent data ({} bytes) from the compressed stream.", dataSize);
			return;
		}
		SetBuffer(NtcMaterialSlot::Latent, dataSize, latentData.data());

		dataSize = convertedSize ? convertedSize : weightDataSize;
		SetBuffer(NtcMaterialSlot::Weight, dataSize, pWeightData);
	}

	std::array<ntc::ShuffleSource, NTC_MAX_CHANNELS> NtcMaterial::BuildShuffleMap() const
	{
		std::array<ntc::ShuffleSource, NTC_MAX_CHANNELS> channelMap;
		// Everything starts as a constant; slots with source data are then pointed at their channel.
		channelMap.fill(ntc::ShuffleSource::Constant(1.f));

		for (int role = 0; role < kNtcChannelRoleCount; ++role)
		{
			const auto semantic = static_cast<NtcChannelRole>(role);
			for (int component = 0; component < kNtcRoleMaxComponents; ++component)
			{
				const int slot = NtcCanonicalSlot(semantic, component);
				if (slot < 0 || slot >= NTC_MAX_CHANNELS)
					continue;

				const int channel = m_ChannelPlan.sourceChannel[role][component];
				channelMap[slot] = channel >= 0
					? ntc::ShuffleSource::Channel(channel)
					: ntc::ShuffleSource::Constant(NtcDefaultChannelValue(semantic, component, m_SlotConstants));
			}
		}

		// No transmission texture in this engine yet: always a constant (0 = opaque medium).
		channelMap[CHANNEL_TRANSMISSION] = ntc::ShuffleSource::Constant(m_SlotConstants.transmission);
		return channelMap;
	}

	void NtcMaterial::LogTextureSet(const char* stage) const
	{
#ifdef BEAR_DEBUG
		LogTextureSetContents(const_cast<ntc::TextureSetWrapper&>(m_TextureSet), stage);
#else
		(void)stage;
#endif
	}
}
