#include "bearpch.h"
#include "NtcMaterial.h"
#include "Texture.h"
#include "NtcChannelMapping.h"
namespace Bear
{
	NtcMaterial::NtcMaterial(RHIDevice& device, const MaterialDescription& desc, const std::unordered_map<int, std::shared_ptr<Texture>>& textures)
		:m_Device(device), m_TextureSet(nullptr)
	{
		std::vector<RHIDescriptorSetLayoutBinding> bindings;
		bindings.push_back({ .binding = NtcMaterialSlot::Latent, .descriptorType = DescriptorType::StorageBuffer, .stageFlags = ShaderStage::Fragment });
		bindings.push_back({ .binding = NtcMaterialSlot::Weight, .descriptorType = DescriptorType::StorageBuffer, .stageFlags = ShaderStage::Fragment });
		bindings.push_back({ .binding = NtcMaterialSlot::Constant, .descriptorType = DescriptorType::UniformBuffer, .stageFlags = ShaderStage::Fragment });
		m_DescriptorSetLayout = device.CreateDescriptorSetLayout(bindings);
		m_DescriptorSets = device.CreateDescriptorSet(m_DescriptorSetLayout);

		ntc::ContextParameters contextParams;
		ntc::Status ntcStatus = ntc::CreateContext(&m_NtcContext, contextParams);
		BEAR_CORE_ASSERT(ntcStatus == ntc::Status::Ok, "Filed to create the ntc context!");
		m_TextureSet = ntc::TextureSetWrapper(m_NtcContext);

		CompressTextures(desc, textures);
		UploadTextures();

	}
	void NtcMaterial::SetBuffer(NtcMaterialSlot slot, size_t dataSize, const void* data)
	{
		switch (slot)
		{
		case NtcMaterialSlot::Latent:
			m_LatentBuffer = m_Device.CreateBuffer(dataSize, BufferUsage::StorageBuffer, true);
			m_DescriptorSets->UpdateBuffer(NtcMaterialSlot::Latent, *m_LatentBuffer);
			m_LatentBuffer->UploadData(data, dataSize);
			latentStreamRange = { 0, dataSize };
			break;
		case NtcMaterialSlot::Weight:
			m_WeightBuffer = m_Device.CreateBuffer(dataSize, BufferUsage::StorageBuffer, true);
			m_DescriptorSets->UpdateBuffer(NtcMaterialSlot::Weight, *m_WeightBuffer);
			m_WeightBuffer->UploadData(data, dataSize);
			break;
		case NtcMaterialSlot::Constant:
			m_ConstantBuffer = m_Device.CreateBuffer(dataSize, BufferUsage::UniformBuffer, true);
			m_DescriptorSets->UpdateBuffer(NtcMaterialSlot::Constant, *m_ConstantBuffer);
			m_ConstantBuffer->UploadData(data, dataSize);
			break;
		default:
			BEAR_CORE_ERROR("Invalid NTC material slot");
			break;
		}
	}
	void NtcMaterial::CompressTextures(const MaterialDescription& desc, const std::unordered_map<int, std::shared_ptr<Texture>>& textures)
	{
		std::string materialName = desc.name;
		std::string filePath = "assets/compress/" + materialName + ".ntc";
		ntc::Status ntcStatus;
		// if the file exists, skip the compression.
		if (std::filesystem::exists(filePath) || m_TextureSet)
		{
			BEAR_CORE_WARN("The compressed texture file already exists, skip the compression.");
			ntc::FileStreamWrapper inputFile(m_NtcContext);
			ntcStatus = m_NtcContext->OpenFile(filePath.c_str(), false, inputFile.ptr());
			BEAR_CORE_ASSERT(ntcStatus == ntc::Status::Ok,
				"Filed to open the compressed texture, code = {} : {}", ntc::StatusToString(ntcStatus), ntc::GetLastErrorMessage());
			m_TextureSet = ntc::TextureSetWrapper(m_NtcContext);
			uint64_t streamSize = inputFile->Size();
			m_CompressedData.resize(streamSize);
			ntc::MemoryStreamWrapper memStream(m_NtcContext);
			ntc::TextureSetMetadataWrapper  textureSetMetaData(m_NtcContext);
			ntc::TextureSetFeatures features;
			ntcStatus = m_NtcContext->CreateCompressedTextureSetFromFile(filePath.c_str(), features, m_TextureSet.ptr());
			BEAR_CORE_ASSERT(ntcStatus == ntc::Status::Ok, "Filed to create the compressed texture set from file, code = {} : {}", ntc::StatusToString(ntcStatus), ntc::GetLastErrorMessage());
			ntcStatus = m_TextureSet->SaveToMemory(m_CompressedData.data(), &streamSize);
			return;
		}
		m_TextureSet = ntc::TextureSetWrapper(m_NtcContext);
		uint8_t numChannels = 0;
		for (const auto& texture : textures | std::views::values)
		{
			numChannels += texture->GetChannels();
		}
		BEAR_CORE_INFO("texture set has {} channels.", numChannels);
		BEAR_CORE_ASSERT(numChannels <= 16, "The number of the texture channels must be less than 16!");
		// keep the same size for all textures.
		uint32_t maxWidth = 0, maxHeight = 0;
		uint32_t minWidth = UINT_MAX, minHeight = UINT_MAX;
		for (const auto& texture : textures | std::views::values)
		{
			maxWidth = std::max(maxWidth, texture->GetWidth());
			maxHeight = std::max(maxHeight, texture->GetHeight());
			minWidth = std::min(minWidth, texture->GetWidth());
			minHeight = std::min(minHeight, texture->GetHeight());
			if ((maxWidth != minWidth) || (maxHeight != minHeight))
			{
				BEAR_CORE_ERROR("The textures have different sizes, the compression may not be optimal.");
			}
		}
		uint32_t width = maxWidth, height = maxHeight;
		ntc::TextureSetDesc textureSetDesc;
		textureSetDesc.channels = numChannels * 2;
		textureSetDesc.width = width;
		textureSetDesc.height = height;
		textureSetDesc.mips = std::min(width, height) > 1 ? static_cast<int>(std::floor(std::log2(std::min(width, height)))) + 1 : 1;

		ntc::TextureSetFeatures features;
		ntcStatus = m_NtcContext->CreateTextureSet(textureSetDesc, features, m_TextureSet.ptr());
		BEAR_CORE_ASSERT(ntcStatus == ntc::Status::Ok,
			"Filed to create the textureSet, code = {} : {}", ntc::StatusToString(ntcStatus), ntc::GetLastErrorMessage());
		
		// The target compression parameters setting.
		float expectedBitsPerPixel = 4.0f; // 4 bits per pixel
		networkVersion = NTC_NETWORK_LARGE;
		float actualBitsPerPixel;
		ntc::LatentShape latentShape;
		ntcStatus = ntc::PickLatentShape(expectedBitsPerPixel, networkVersion, actualBitsPerPixel, latentShape);
		BEAR_CORE_ASSERT(ntcStatus == ntc::Status::Ok,
			"Filed to pick the suitable latent shape, code = {} : {}", ntc::StatusToString(ntcStatus), ntc::GetLastErrorMessage());
		ntcStatus = m_TextureSet->SetLatentShape(latentShape, networkVersion);
		BEAR_CORE_ASSERT(ntcStatus == ntc::Status::Ok, 
			"Filed to set the latent shape, code = {} : {}", ntc::StatusToString(ntcStatus), ntc::GetLastErrorMessage());

		int firstChannel = 0;
		for (const auto& texture : textures | std::views::values)
		{
			numChannels = texture->GetChannels();
			ntc::ColorSpace colorSpaces[4] = { ntc::ColorSpace::sRGB, ntc::ColorSpace::sRGB, ntc::ColorSpace::sRGB, ntc::ColorSpace::Linear };
			ntc::ITextureMetadata* textureMetadata = m_TextureSet->AddTexture();
			textureMetadata->SetName(desc.name.c_str());
			textureMetadata->SetChannels(firstChannel, numChannels);
			textureMetadata->SetRgbColorSpace(ntc::ColorSpace::sRGB);
			textureMetadata->SetAlphaColorSpace(ntc::ColorSpace::Linear);

			// write the pixel data to the texture set
			ntc::WriteChannelsParameters writeParams;
			writeParams.mipLevel = 0;
			writeParams.firstChannel = firstChannel;
			writeParams.numChannels = numChannels;
			writeParams.pData = static_cast<unsigned char const*>(texture->GetImageData());
			writeParams.addressSpace = ntc::AddressSpace::Host;
			writeParams.width = width;
			writeParams.height = height;
			writeParams.pixelStride = static_cast<size_t>(numChannels);
			writeParams.rowPitch = static_cast<size_t>(width * numChannels);
			writeParams.channelFormat = ntc::ChannelFormat::UNORM8;
			writeParams.srcColorSpaces = colorSpaces;
			writeParams.dstColorSpaces = colorSpaces;

			ntcStatus = m_TextureSet->WriteChannels(writeParams);
			BEAR_CORE_ASSERT(ntcStatus == ntc::Status::Ok,
				"Filed to write the pixel data into textureSet, code = {} : {}", ntc::StatusToString(ntcStatus), ntc::GetLastErrorMessage());

			firstChannel += numChannels;
		}
		// generate mipmaps for the input textures.
		ntcStatus = m_TextureSet->GenerateMips();
		BEAR_CORE_ASSERT(ntcStatus == ntc::Status::Ok, "Failed to generate the mipmaps.");
		ntc::CompressionSettings compSettings;
		ntcStatus = m_TextureSet->BeginCompression(compSettings);
		BEAR_CORE_ASSERT(ntcStatus == ntc::Status::Ok,
			"Filed to begin to compress, code = {} : {}", ntc::StatusToString(ntcStatus), ntc::GetLastErrorMessage());

		ntc::CompressionStats stats;
		do
		{
			ntcStatus = m_TextureSet->RunCompressionSteps(&stats);
			if (ntcStatus == ntc::Status::Ok || ntcStatus == ntc::Status::Incomplete)
			{
				printf("\rCompression step %d/%d (%.2f ms/step), loss = %.6f (PSNR %.2f dB), LR: net %.6f, grid %.6f",
					stats.currentStep, compSettings.trainingSteps,
					stats.millisecondsPerStep,
					stats.loss, ntc::LossToPSNR(stats.loss),
					stats.learningRate, stats.learningRate * (compSettings.gridLearningRate / compSettings.networkLearningRate));
				fflush(stdout);
			}
			else
			{
				BEAR_CORE_ERROR("Filed to run the compression step, code = {} : {}", ntc::StatusToString(ntcStatus), ntc::GetLastErrorMessage());
			}
		} while (ntcStatus == ntc::Status::Incomplete);
		printf("\n");
		BEAR_CORE_ASSERT(ntcStatus == ntc::Status::Ok,
			"Filed to compress texture, code = {} : {}", ntc::StatusToString(ntcStatus), ntc::GetLastErrorMessage());

		ntcStatus = m_TextureSet->FinalizeCompression();
		BEAR_CORE_ASSERT(ntcStatus == ntc::Status::Ok,
			"Filed to end compression steps, code = {} : {}", ntc::StatusToString(ntcStatus), ntc::GetLastErrorMessage());
		// save the compressed texture set to a file for debug
		for (int index = 0; index < m_TextureSet->GetTextureCount(); ++index)
		{
			ntc::ITextureMetadata* texMeta = m_TextureSet->GetTexture(index);
			BEAR_CORE_INFO("Texture[{}] '{}': channels {}..{}, block compression {}, RGB space {}, Alpha space {}",
				index, texMeta->GetName(),
				texMeta->GetFirstChannel(), texMeta->GetFirstChannel() + texMeta->GetNumChannels() - 1,
				ntc::BlockCompressedFormatToString(texMeta->GetBlockCompressedFormat()),
				ntc::ColorSpaceToString(texMeta->GetRgbColorSpace()),
				ntc::ColorSpaceToString(texMeta->GetAlphaColorSpace()));
		}
		ntcStatus = m_TextureSet->SaveToFile(filePath.c_str());
		BEAR_CORE_ASSERT(ntcStatus == ntc::Status::Ok,
			"Filed to save the compressed texture, code = {} : {}", ntc::StatusToString(ntcStatus), ntc::GetLastErrorMessage());
		// save the compressed texture set to memory for uploading to GPU
		uint64_t streamSize = m_TextureSet->GetOutputStreamSize();
		m_CompressedData.resize(streamSize);
		ntcStatus = m_TextureSet->SaveToMemory(m_CompressedData.data(), &streamSize);
		BEAR_CORE_ASSERT(ntcStatus == ntc::Status::Ok,
			"Filed to save the compressed texture to memory, code = {} : {}", ntc::StatusToString(ntcStatus), ntc::GetLastErrorMessage());
	}
	void NtcMaterial::UploadTextures()
	{
		ntc::MemoryStreamWrapper memStream(m_NtcContext);
		ntc::Status ntcStatus = m_NtcContext->OpenReadOnlyMemory(m_CompressedData.data(), m_CompressedData.size(), memStream.ptr());
		BEAR_CORE_ASSERT(ntcStatus == ntc::Status::Ok,
			"Filed to open the memory stream, code = {} : {}", ntc::StatusToString(ntcStatus), ntc::GetLastErrorMessage());
		ntc::TextureSetMetadataWrapper textureSetMetadata(m_NtcContext);
		ntcStatus = m_NtcContext->CreateTextureSetMetadataFromStream(memStream, textureSetMetadata.ptr());
		BEAR_CORE_ASSERT(ntcStatus == ntc::Status::Ok,
			"Filed to create the texture set metadata from stream, code = {} : {}", ntc::StatusToString(ntcStatus), ntc::GetLastErrorMessage());
		auto channelMap = CreateChannelMap();
		textureSetMetadata->ShuffleInferenceOutputs(channelMap.data());
		void const* pWeightData = nullptr;
		size_t weightDataSize = 0;
		size_t convertedSize = 0;
		ntcStatus = textureSetMetadata->GetInferenceWeights(ntc::InferenceWeightType::GenericInt8, &pWeightData, &weightDataSize, &convertedSize);
		BEAR_CORE_ASSERT(ntcStatus == ntc::Status::Ok,
			"Filed to get the inference weights, code = {} : {}", ntc::StatusToString(ntcStatus), ntc::GetLastErrorMessage());

		ntc::StreamRange latentRange;
		ntcStatus = textureSetMetadata->GetStreamRangeForLatents(0, textureSetMetadata->GetDesc().mips, latentRange);
		BEAR_CORE_ASSERT(ntcStatus == ntc::Status::Ok,
			"Filed to get the stream range for latents, code = {} : {}", ntc::StatusToString(ntcStatus), ntc::GetLastErrorMessage());

		ntc::InferenceData inferenceData;
		ntcStatus = m_NtcContext->MakeInferenceData(textureSetMetadata, latentRange, ntc::InferenceWeightType::GenericInt8, &inferenceData);
		BEAR_CORE_ASSERT(ntcStatus == ntc::Status::Ok,
			"Filed to make the inference data, code = {} : {}", ntc::StatusToString(ntcStatus), ntc::GetLastErrorMessage());

		size_t dataSize = sizeof(inferenceData.constants);
		SetBuffer(NtcMaterialSlot::Constant, dataSize, &inferenceData);
		dataSize = latentRange.size;
		std::vector<uint8_t> latentData(dataSize);
		memStream->Seek(latentRange.offset);
		auto status = memStream->Read(latentData.data(), dataSize);
		BEAR_CORE_ASSERT(status, "Failed to read latent data from input file!");
		SetBuffer(NtcMaterialSlot::Latent, dataSize, latentData.data());
		dataSize = convertedSize ? convertedSize : weightDataSize;
		SetBuffer(NtcMaterialSlot::Weight, dataSize, pWeightData);
	}
	std::array<ntc::ShuffleSource, NTC_MAX_CHANNELS> NtcMaterial::CreateChannelMap()
	{
		std::array<ntc::ShuffleSource, NTC_MAX_CHANNELS> channelMap;
		channelMap.fill(ntc::ShuffleSource::Constant(1.f));
		channelMap[CHANNEL_NORMAL + 0] = ntc::ShuffleSource::Constant(0.5f);
		channelMap[CHANNEL_NORMAL + 1] = ntc::ShuffleSource::Constant(0.5f);
		// base color.
		channelMap[CHANNEL_BASE_COLOR + 0] = ntc::ShuffleSource::Channel(0);
		channelMap[CHANNEL_BASE_COLOR + 1] = ntc::ShuffleSource::Channel(1);
		channelMap[CHANNEL_BASE_COLOR + 2] = ntc::ShuffleSource::Channel(2);
		channelMap[CHANNEL_OPACITY] = ntc::ShuffleSource::Channel(3);
		return channelMap;
	}
}

