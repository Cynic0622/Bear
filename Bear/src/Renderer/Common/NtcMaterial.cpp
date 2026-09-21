#include "bearpch.h"
#include "NtcMaterial.h"
#include "Texture.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace Bear
{
	namespace
	{
		// 压缩参数。它们参与缓存文件名的指纹，改了以后不会命中旧结果。
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

		// NTC 的失败基本都可恢复（缓存文件不合法、参数越界），而 BEAR_CORE_ERROR 结尾带
		// __debugbreak()，所以这里只记录错误、不打断执行。
		void LogNtcFailure(const char* what, ntc::Status status)
		{
			::Bear::Log::GetCoreLogger()->error("NTC: {} failed, code = {} : {}",
				what, ntc::StatusToString(status), ntc::GetLastErrorMessage());
		}

		// 语义通道在纹理集里的色彩空间：基色/自发光是 sRGB 数据，其余是线性数据。
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

		// 纹理集里某个通道的色彩空间（从布局计划反查它属于哪个语义）。
		ntc::ColorSpace ChannelColorSpace(const NtcChannelPlan& plan, int channel)
		{
			for (int role = 0; role < kNtcChannelRoleCount; ++role)
				for (int component = 0; component < kNtcRoleMaxComponents; ++component)
					if (plan.sourceChannel[role][component] == channel)
						return RoleColorSpace(static_cast<NtcChannelRole>(role));
			return ntc::ColorSpace::Linear;
		}
	}

	NtcMaterial::NtcMaterial(RHIDevice& device, const MaterialDescription& desc, const std::unordered_map<int, std::shared_ptr<Texture>>& textures)
		:m_Device(device), m_TextureSet(nullptr)
	{
		m_DescriptorSetLayout = device.CreateDescriptorSetLayout(GetDescriptorSetLayoutBinding());
		m_DescriptorSets = device.CreateDescriptorSet(m_DescriptorSetLayout);

		ntc::ContextParameters contextParams;
		ntc::Status ntcStatus = ntc::CreateContext(&m_NtcContext, contextParams);
		if (ntcStatus != ntc::Status::Ok)
		{
			LogNtcFailure("CreateContext", ntcStatus);
			return;
		}
		m_TextureSet = ntc::TextureSetWrapper(m_NtcContext);

		// 先把通道布局定下来：写入计划与渲染侧的槽位映射都由它派生。
		BuildChannelPlan(desc, textures);
		CompressTextures(desc, textures);
		UploadTextures();
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
			// GetChannels() 是图片文件本身的有意义分量数（1..4），与补齐成 RGBA 的缓冲区区分开。
			source.channels = static_cast<uint8_t>(std::clamp<int>(texture->GetChannels(), 1, 4));
			sources.push_back(source);
		};

		addSource(NtcChannelRole::BaseColor, desc.baseColorTextureIndex);
		addSource(NtcChannelRole::Metalness, desc.metallicRoughnessTextureIndex); // metallicRoughness 贴图
		addSource(NtcChannelRole::Normal, desc.normalTextureIndex);
		addSource(NtcChannelRole::Occlusion, desc.occlusionTextureIndex);
		addSource(NtcChannelRole::Emissive, desc.emissiveTextureIndex);

		m_ChannelPlan = BuildNtcChannelPlan(sources, desc.isTransparent);

		// 没有贴图的语义用材质因子兜底。着色器读基色/自发光时不做色彩空间解码（与贴图路径的
		// 存储值保持一致），所以这里把线性因子编码成 sRGB 存储值。
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

	std::string NtcMaterial::GetCompressedFilePath(const MaterialDescription& desc, const std::unordered_map<int, std::shared_ptr<Texture>>& textures) const
	{
		// 文件名 = 材质名 + 布局版本 + 源数据指纹。布局方式、源贴图或压缩参数变了都不会命中旧结果，
		// 也不会出现不同材质（例如 Sponza 里几个未命名材质）共用同一个文件的情况。
		// 注意：贴图像素被原地修改、但尺寸与通道数不变的情况检测不到，见后续的缓存失效任务。
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
		mix(desc.isTransparent ? 1ull : 0ull);
		// 材质因子会作为常量进入纹理集，所以它们也影响结果（尤其是完全没有贴图的材质）。
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

		// 材质名可能为空或含路径分隔符，统一清洗成安全的名字。
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

	void NtcMaterial::CompressTextures(const MaterialDescription& desc, const std::unordered_map<int, std::shared_ptr<Texture>>& textures)
	{
		if (!m_NtcContext)
			return;

		if (m_ChannelPlan.totalChannels > NTC_MAX_CHANNELS)
		{
			::Bear::Log::GetCoreLogger()->error("NTC: material '{}' needs {} channels but the limit is {}.",
				desc.name, m_ChannelPlan.totalChannels, NTC_MAX_CHANNELS);
			return;
		}

		const std::string filePath = GetCompressedFilePath(desc, textures);
		ntc::Status ntcStatus;

		// --- 已有压缩结果就复用 ---
		if (std::filesystem::exists(filePath))
		{
			BEAR_CORE_INFO("NTC: '{}' is already compressed, loading '{}'.", desc.name, filePath);

			ntc::FileStreamWrapper inputFile(m_NtcContext);
			ntcStatus = m_NtcContext->OpenFile(filePath.c_str(), false, inputFile.ptr());
			if (ntcStatus != ntc::Status::Ok) { LogNtcFailure("OpenFile", ntcStatus); return; }

			m_TextureSet = ntc::TextureSetWrapper(m_NtcContext);
			ntc::TextureSetFeatures features;
			ntcStatus = m_NtcContext->CreateCompressedTextureSetFromStream(inputFile, features, m_TextureSet.ptr());
			if (ntcStatus != ntc::Status::Ok) { LogNtcFailure("CreateCompressedTextureSetFromStream", ntcStatus); return; }

			size_t streamSize = static_cast<size_t>((std::max)(inputFile->Size(), m_TextureSet->GetOutputStreamSize()));
			m_CompressedData.resize(streamSize);
			ntcStatus = m_TextureSet->SaveToMemory(m_CompressedData.data(), &streamSize);
			if (ntcStatus != ntc::Status::Ok) { LogNtcFailure("SaveToMemory", ntcStatus); return; }

			LogTextureSet("loaded");
			return;
		}

		// --- 压缩 ---
		m_TextureSet = ntc::TextureSetWrapper(m_NtcContext);

		// 没有任何贴图的材质（例如 TransmissionTest 里的玻璃材质）：仍然要有一个合法的纹理集，
		// 这里塞一个 1x1 的哑通道占位，所有语义都由常量提供，该通道不会被任何槽位引用。
		if (m_ChannelPlan.writes.empty())
		{
			BEAR_CORE_INFO("NTC: material '{}' has no textures, compressing a constant texture set.", desc.name);
			m_ChannelPlan.writes.push_back(NtcChannelWrite{ -1, 0, 1, 0 });
			m_ChannelPlan.totalChannels = 1;
		}

		// 纹理集尺寸取输入里最大的那张，NTC 会把小图重采样上去（CopyImageKernel 走双线性重采样）。
		uint32_t width = 1, height = 1;
		for (const NtcChannelWrite& write : m_ChannelPlan.writes)
		{
			const Texture* texture = ResolveTexture(textures, write.textureIndex);
			if (!texture)
				continue;
			width = (std::max)(width, texture->GetWidth());
			height = (std::max)(height, texture->GetHeight());
		}

		ntc::TextureSetDesc textureSetDesc;
		textureSetDesc.channels = m_ChannelPlan.totalChannels;
		textureSetDesc.width = static_cast<int>(width);
		textureSetDesc.height = static_cast<int>(height);
		// TODO: mip 链固定为 1（着色器按 LOD 采样，但文件里只有 mip0）。开启前需要重新评估
		//       压缩时间、文件大小与画质。
		textureSetDesc.mips = 1;

		ntc::TextureSetFeatures features;
		ntcStatus = m_NtcContext->CreateTextureSet(textureSetDesc, features, m_TextureSet.ptr());
		if (ntcStatus != ntc::Status::Ok) { LogNtcFailure("CreateTextureSet", ntcStatus); return; }

		// 目标码率与网络规模。
		networkVersion = kNtcNetworkVersion;
		float actualBitsPerPixel = 0.f;
		ntc::LatentShape latentShape;
		ntcStatus = ntc::PickLatentShape(kNtcExpectedBitsPerPixel, networkVersion, actualBitsPerPixel, latentShape);
		if (ntcStatus != ntc::Status::Ok) { LogNtcFailure("PickLatentShape", ntcStatus); return; }
		ntcStatus = m_TextureSet->SetLatentShape(latentShape, networkVersion);
		if (ntcStatus != ntc::Status::Ok) { LogNtcFailure("SetLatentShape", ntcStatus); return; }

		BEAR_CORE_INFO("NTC: compressing '{}' ({}x{}, {} channels, target {:.2f} bpp -> {:.2f}).",
			desc.name, width, height, m_ChannelPlan.totalChannels, kNtcExpectedBitsPerPixel, actualBitsPerPixel);

		const std::vector<uint8_t> placeholderPixel{ 0, 0, 0, 255 };
		for (const NtcChannelWrite& write : m_ChannelPlan.writes)
		{
			const Texture* texture = ResolveTexture(textures, write.textureIndex);

			ntc::WriteChannelsParameters writeParams;
			writeParams.mipLevel = 0;
			writeParams.firstChannel = write.firstChannel;
			writeParams.numChannels = write.numChannels;
			writeParams.addressSpace = ntc::AddressSpace::Host;
			writeParams.channelFormat = ntc::ChannelFormat::UNORM8;
			writeParams.pixelStride = 4; // Texture::GetImageData() 永远按 RGBA 补齐成 4 字节/像素
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
				colorSpaces[i] = ChannelColorSpace(m_ChannelPlan, write.firstChannel + i);
			writeParams.srcColorSpaces = colorSpaces;
			writeParams.dstColorSpaces = colorSpaces;

			ntcStatus = m_TextureSet->WriteChannels(writeParams);
			if (ntcStatus != ntc::Status::Ok) { LogNtcFailure("WriteChannels", ntcStatus); return; }
		}

		// 按源贴图登记元数据（同一张贴图的连续通道段合并成一条）。
		{
			struct TextureRange { int textureIndex; int firstChannel; int numChannels; };
			std::vector<TextureRange> ranges;
			for (const NtcChannelWrite& write : m_ChannelPlan.writes)
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
				ntc::ITextureMetadata* metadata = m_TextureSet->AddTexture();
				if (!metadata)
					continue;
				const Texture* texture = ResolveTexture(textures, range.textureIndex);
				if (texture)
					metadata->SetName(texture->GetName().c_str());
				metadata->SetChannels(range.firstChannel, range.numChannels);
				metadata->SetRgbColorSpace(ChannelColorSpace(m_ChannelPlan, range.firstChannel));
				metadata->SetAlphaColorSpace(range.numChannels > 3
					? ChannelColorSpace(m_ChannelPlan, range.firstChannel + 3)
					: ntc::ColorSpace::Linear);
			}
		}

		// 输入贴图的 mip 链（纹理集本身目前仍是单 mip）。
		ntcStatus = m_TextureSet->GenerateMips();
		if (ntcStatus != ntc::Status::Ok) { LogNtcFailure("GenerateMips", ntcStatus); return; }

		ntc::CompressionSettings compSettings;
		compSettings.trainingSteps = kNtcTrainingSteps;
		ntcStatus = m_TextureSet->BeginCompression(compSettings);
		if (ntcStatus != ntc::Status::Ok) { LogNtcFailure("BeginCompression", ntcStatus); return; }

		ntc::CompressionStats stats;
		do
		{
			ntcStatus = m_TextureSet->RunCompressionSteps(&stats);
			if (ntcStatus != ntc::Status::Ok && ntcStatus != ntc::Status::Incomplete)
			{
				LogNtcFailure("RunCompressionSteps", ntcStatus);
				return;
			}
			BEAR_CORE_INFO("NTC '{}': step {}/{}, loss = {:.6f} (PSNR {:.2f} dB)",
				desc.name, stats.currentStep, compSettings.trainingSteps, stats.loss, ntc::LossToPSNR(stats.loss));
		} while (ntcStatus == ntc::Status::Incomplete);

		ntcStatus = m_TextureSet->FinalizeCompression();
		if (ntcStatus != ntc::Status::Ok) { LogNtcFailure("FinalizeCompression", ntcStatus); return; }

		LogTextureSet("compressed");

		ntcStatus = m_TextureSet->SaveToFile(filePath.c_str());
		if (ntcStatus != ntc::Status::Ok) { LogNtcFailure("SaveToFile", ntcStatus); return; }

		size_t streamSize = static_cast<size_t>(m_TextureSet->GetOutputStreamSize());
		m_CompressedData.resize(streamSize);
		ntcStatus = m_TextureSet->SaveToMemory(m_CompressedData.data(), &streamSize);
		if (ntcStatus != ntc::Status::Ok) { LogNtcFailure("SaveToMemory", ntcStatus); return; }
	}

	void NtcMaterial::UploadTextures()
	{
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

		// 把纹理集的通道重排到渲染端读取的语义槽位；缺数据的槽位填常量。
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
		// 默认全是常量，随后把有源数据的语义槽位接到纹理集对应的通道上。
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

		// 本引擎还没有透射贴图，固定走常量（0 = 不透明介质）。
		channelMap[CHANNEL_TRANSMISSION] = ntc::ShuffleSource::Constant(m_SlotConstants.transmission);
		return channelMap;
	}

	void NtcMaterial::LogTextureSet(const char* stage) const
	{
#ifdef BEAR_DEBUG
		if (!m_TextureSet)
			return;
		for (int index = 0; index < m_TextureSet->GetTextureCount(); ++index)
		{
			ntc::ITextureMetadata* textureMetadata = m_TextureSet->GetTexture(index);
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
#else
		(void)stage;
#endif
	}
}
