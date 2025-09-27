#include "bearpch.h"
#include "Resource.h"

#include <filesystem>

#include "AssetLoader.h"
#include "Instance.h"
#include "Material.h"
#include "RHI/RHIDevice.h"
#include "Texture.h"
#include "Mesh.h"
#include "NtcChannelMapping.h"
#include "NtcMaterial.h"
#include "PbrMaterial.h"
namespace Bear
{
	Resource::Resource(RHIDevice& device)
		:m_Device(device)
	{
	}
	Resource::Resource(RenderContext* context)
		:m_Device(*context->device)
	{
		m_RenderContext = context;
	}
	std::shared_ptr<Texture> Resource::CreateTexture(const ImageDescription& imageDesc, const SamplerDescription& samplerDesc)
	{
		return std::make_shared<Texture>(m_Device, imageDesc, samplerDesc);
	}
	std::shared_ptr<Mesh> Resource::CreateMesh(const PrimitiveDescription& desc)
	{
		return std::make_shared<Mesh>(m_Device, desc.vertices, desc.indices);
	}
	std::shared_ptr<Material> Resource::CreateMaterial(const MaterialDescription& desc, const std::vector<std::shared_ptr<Texture>>& images)
	{
		auto material = std::make_shared<PbrMaterial>(m_Device);
		auto bindTex = [&](int slot, int imageIndex)
		{
			if (imageIndex >= 0 && static_cast<size_t>(imageIndex) < images.size() && images[imageIndex]) {
				material->SetTexture(slot, images[imageIndex]); // bind the texture to the material
			}
			else
			{
				material->SetTexture(slot, GetDefaultTexture(slot)); // bind default texture if not available
			}
		};
		bindTex(MaterialSlot::BaseColor, desc.baseColorTextureIndex);
		bindTex(MaterialSlot::MetallicRoughness, desc.metallicRoughnessTextureIndex);
		bindTex(MaterialSlot::Normal, desc.normalTextureIndex);
		bindTex(MaterialSlot::Occlusion, desc.occlusionTextureIndex);
		bindTex(MaterialSlot::Emissive, desc.emissiveTextureIndex);

		material->SetParam("baseColorFactor", desc.baseColorFactor);
		material->SetParam("metallicFactor", desc.metallicFactor);
		material->SetParam("roughnessFactor", desc.roughnessFactor);
		material->SetParam("emissiveFactor", desc.emissiveFactor);
		material->SetParam("normalScale", 1.0f); // default normal scale
		material->SetParam("occlusionStrength", 1.0f); // default occlusion strength
		material->SetTransparent(desc.isTransparent);
		return material;
	}
	Resources Resource::CreateResources(const ModelDescription& desc)
	{
		Resources res{};
		res.Materials.resize(desc.materials.size());
		// be careful when using smart pointers pointing temp objects, they will be destroyed after this function returns.
		std::vector<std::shared_ptr<Texture>> textures(desc.textures.size());
		for (size_t i = 0; i < desc.textures.size(); ++i)
		{
			if (!desc.samplers.empty())
			{
				textures[i] = CreateTexture(desc.images[desc.textures[i].imageIndex], desc.samplers[desc.textures[i].samplerIndex == -1 ? 0 : desc.textures[i].samplerIndex]);
			}
			else
			{
				textures[i] = CreateTexture(desc.images[desc.textures[i].imageIndex], SamplerDescription{}); // use default sampler if not specified
			}
		}

		for (size_t i = 0; i < desc.materials.size(); ++i)
		{
			res.Materials[i] = CreateMaterial(desc.materials[i], textures);
		}

		res.Meshes.resize(desc.primitives.size());
		for (size_t i = 0; i < desc.primitives.size(); ++i)
		{
			res.Meshes[i] = CreateMesh(desc.primitives[i]);
		}
		if (m_TextureCompressed)
		{
			CompressTexture(desc);
			for (size_t i = 0; i < desc.materials.size(); ++i)
			{
				res.Materials[i] = CreateNtcMaterial();
			}

		}
		return res;
	}
	std::shared_ptr<Texture> Resource::GetDefaultTexture(int bindingSlot)
	{
		// cache per-slot defaults to avoid reallocating
		

		if (bindingSlot >= 0 && bindingSlot < (int)m_DefaultTextures.size() && m_DefaultTextures[bindingSlot])
			return m_DefaultTextures[bindingSlot];

		uint8_t pixel[4] = { 0xFF, 0xFF, 0xFF, 0xFF }; // default white

		switch (bindingSlot)
		{
		case MaterialSlot::BaseColor:
			// white opaque
			pixel[0] = 0xFF; pixel[1] = 0xFF; pixel[2] = 0xFF; pixel[3] = 0xFF;
			break;
		case MaterialSlot::MetallicRoughness:
			// roughness = 1.0 -> G = 255, metallic = 0.0 -> B = 0
			pixel[0] = 0xFF; pixel[1] = 0xFF; pixel[2] = 0x00; pixel[3] = 0xFF;
			break;
		case MaterialSlot::Normal:
			// neutral normal = (0.5, 0.5, 1.0) -> (128,128,255)
			pixel[0] = 128; pixel[1] = 128; pixel[2] = 255; pixel[3] = 0xFF;
			break;
		case MaterialSlot::Occlusion:
			// occlusion = 1.0 -> R = 255
			pixel[0] = 0xFF; pixel[1] = 0xFF; pixel[2] = 0xFF; pixel[3] = 0xFF;
			break;
		case MaterialSlot::Emissive:
			// black (no emission)
			pixel[0] = 0x00; pixel[1] = 0x00; pixel[2] = 0x00; pixel[3] = 0xFF;
			break;
		default:
			pixel[0] = 0xFF; pixel[1] = 0xFF; pixel[2] = 0xFF; pixel[3] = 0xFF;
			break;
		}

		auto tex = std::make_shared<Texture>(m_Device, 1, 1, 4, pixel);
		if (bindingSlot >= 0 && bindingSlot < (int)m_DefaultTextures.size())
			m_DefaultTextures[bindingSlot] = tex;
		return tex;
	}
	void Resource::CompressTexture(const ModelDescription& modelDesc)
	{
		ntc::ContextParameters contextParams;
		ntc::Status ntcStatus;
		BEAR_CORE_ASSERT((ntcStatus = ntc::CreateContext(&m_NtcContext, contextParams)) == ntc::Status::Ok, "Filed to create the ntc context!");

		std::string filePath = "assets/compress/compressed.ntc";
		// if the file exists, skip the compression.
		if (std::filesystem::exists(filePath))
		{
			BEAR_CORE_WARN("The compressed texture file already exists, skip the compression.");
			return;
		}
		ntc::TextureSetWrapper textureSet(m_NtcContext);
		uint8_t numChannels = 0;
		for (auto& desc : modelDesc.textures)
		{
			numChannels += modelDesc.images[desc.imageIndex].channels;
		}
		BEAR_CORE_INFO("texture set has {} channels.", numChannels);
		BEAR_CORE_ASSERT(numChannels <= 16, "The number of the texture channels must be less than 16!");

		uint32_t height = modelDesc.images[0].height;
		uint32_t width = modelDesc.images[0].width;
		ntc::TextureSetDesc textureSetDesc;
		textureSetDesc.channels = numChannels;
		textureSetDesc.width = width;
		textureSetDesc.height = height;
		// TODO: generate mipmaps
		// textureSetDesc.mips = 16;

		ntc::TextureSetFeatures features;
		BEAR_CORE_ASSERT((ntcStatus = m_NtcContext->CreateTextureSet(textureSetDesc, features, textureSet.ptr())) == ntc::Status::Ok,
			"Filed to create the textureSet!");
		// ntcStatus = textureSet->GenerateMips();
		// BEAR_CORE_ASSERT(ntcStatus == ntc::Status::Ok, "Failed to generate the mipmaps.");
		// The target compression parameters setting.
		float expectedBitsPerPixel = 4.0f; // 4 bits per pixel
		int networkVersion = NTC_NETWORK_LARGE;
		float actualBitsPerPixel;
		ntc::LatentShape latentShape;
		ntcStatus = ntc::PickLatentShape(expectedBitsPerPixel, networkVersion, actualBitsPerPixel, latentShape);
		BEAR_CORE_ASSERT(ntcStatus == ntc::Status::Ok, "Filed to pick the suitable latent shape, code = {} : {}", ntc::StatusToString(ntcStatus), ntc::GetLastErrorMessage());
		ntcStatus = textureSet->SetLatentShape(latentShape, networkVersion);
		BEAR_CORE_ASSERT(ntcStatus == ntc::Status::Ok, "Filed to set the latent shape, code = {} : {}", ntc::StatusToString(ntcStatus), ntc::GetLastErrorMessage());

		int firstChannel = 0;
		for (auto& desc : modelDesc.textures)
		{
			int numChannels = modelDesc.images[desc.imageIndex].channels;
			ntc::ColorSpace colorSpaces[4] = { ntc::ColorSpace::sRGB, ntc::ColorSpace::sRGB, ntc::ColorSpace::sRGB, ntc::ColorSpace::Linear };
			ntc::ITextureMetadata* textureMetadata = textureSet->AddTexture();
			textureMetadata->SetName(desc.name.c_str());
			textureMetadata->SetChannels(firstChannel, numChannels);
			// textureMetadata->SetBlockCompressedFormat(ntc::BlockCompressedFormat::None);
			textureMetadata->SetRgbColorSpace(ntc::ColorSpace::sRGB);
			textureMetadata->SetAlphaColorSpace(ntc::ColorSpace::Linear);

			// write the pixel data to the texture set
			ntc::WriteChannelsParameters writeParams;
			writeParams.mipLevel = 0;
			writeParams.firstChannel = firstChannel;
			writeParams.numChannels = numChannels;
			writeParams.pData = modelDesc.images[desc.imageIndex].pixels.data();
			writeParams.addressSpace = ntc::AddressSpace::Host;
			writeParams.width = width;
			writeParams.height = height;
			writeParams.pixelStride = static_cast<size_t>(numChannels);
			writeParams.rowPitch = static_cast<size_t>(width * numChannels);
			writeParams.channelFormat = ntc::ChannelFormat::UNORM8;
			writeParams.srcColorSpaces = colorSpaces;
			writeParams.dstColorSpaces = colorSpaces;

			ntcStatus = textureSet->WriteChannels(writeParams);
			BEAR_CORE_ASSERT(ntcStatus == ntc::Status::Ok, "Filed to write the pixel data into textureSet, code = {} : {}",
				ntc::StatusToString(ntcStatus), ntc::GetLastErrorMessage());
			
			firstChannel += numChannels;
		}
		ntc::CompressionSettings compSettings;
		ntcStatus = textureSet->BeginCompression(compSettings);
		BEAR_CORE_ASSERT(ntcStatus == ntc::Status::Ok, "Filed to begin to compress, code = {} : {}", ntc::StatusToString(ntcStatus), ntc::GetLastErrorMessage());

		ntc::CompressionStats stats;
		do
		{
			ntcStatus = textureSet->RunCompressionSteps(&stats);
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
		}
		while (ntcStatus == ntc::Status::Incomplete);
		printf("\n");
		BEAR_CORE_ASSERT(ntcStatus == ntc::Status::Ok, "Filed to compress texture, code = {} : {}", ntc::StatusToString(ntcStatus), ntc::GetLastErrorMessage());

		ntcStatus = textureSet->FinalizeCompression();
		BEAR_CORE_ASSERT(ntcStatus == ntc::Status::Ok, "Filed to end compression steps, code = {} : {}", ntc::StatusToString(ntcStatus), ntc::GetLastErrorMessage());

		// save the compressed texture set to a file for debug
		for (int index = 0; index < textureSet->GetTextureCount(); ++index)
		{
			ntc::ITextureMetadata* texMeta = textureSet->GetTexture(index);
			BEAR_CORE_INFO("Texture[{}] '{}': channels {}..{}, block compression {}, RGB space {}, Alpha space {}",
				index, texMeta->GetName(),
				texMeta->GetFirstChannel(), texMeta->GetFirstChannel() + texMeta->GetNumChannels() - 1,
				ntc::BlockCompressedFormatToString(texMeta->GetBlockCompressedFormat()),
				ntc::ColorSpaceToString(texMeta->GetRgbColorSpace()),
				ntc::ColorSpaceToString(texMeta->GetAlphaColorSpace()));
		}
		ntcStatus = textureSet->SaveToFile("assets/compress/compressed.ntc");
		BEAR_CORE_ASSERT(ntcStatus == ntc::Status::Ok, "Filed to save the compressed texture, code = {} : {}", ntc::StatusToString(ntcStatus), ntc::GetLastErrorMessage());
	}
	std::shared_ptr<Material> Resource::CreateNtcMaterial()
	{
		// upload the compressed texture set to GPU
		ntc::FileStreamWrapper inputFile(m_NtcContext);

		// set the constanet path temporarily.
		ntc::Status ntcStatus = m_NtcContext->OpenFile("assets/compress/compressed.ntc", false, inputFile.ptr());
		BEAR_CORE_ASSERT(ntcStatus == ntc::Status::Ok, "Filed to open the compressed texture, code = {} : {}", ntc::StatusToString(ntcStatus), ntc::GetLastErrorMessage());
		ntc::TextureSetMetadataWrapper textureSetMetadata(m_NtcContext);
		
		ntcStatus = m_NtcContext->CreateTextureSetMetadataFromStream(inputFile, textureSetMetadata.ptr());
		BEAR_CORE_ASSERT(ntcStatus == ntc::Status::Ok, "Filed to create the texture metadata from stream, code = {} : {}", ntc::StatusToString(ntcStatus), ntc::GetLastErrorMessage());
		// BEAR_CORE_INFO(textureSetMetadata->GetNetworkVersion());
		m_NtcChannelMap.fill(ntc::ShuffleSource::Constant(1.f));
		m_NtcChannelMap[CHANNEL_NORMAL + 0] = ntc::ShuffleSource::Constant(0.5f);
		m_NtcChannelMap[CHANNEL_NORMAL + 1] = ntc::ShuffleSource::Constant(0.5f);
		// base color.
		m_NtcChannelMap[CHANNEL_BASE_COLOR + 0] = ntc::ShuffleSource::Channel(0);
		m_NtcChannelMap[CHANNEL_BASE_COLOR + 1] = ntc::ShuffleSource::Channel(1);
		m_NtcChannelMap[CHANNEL_BASE_COLOR + 2] = ntc::ShuffleSource::Channel(2);
		m_NtcChannelMap[CHANNEL_OPACITY] = ntc::ShuffleSource::Channel(3);
		textureSetMetadata->ShuffleInferenceOutputs(m_NtcChannelMap.data());
		auto const weightType = ntc::InferenceWeightType::GenericInt8;
		void const* pWeightData = nullptr;
		size_t weightDataSize = 0;
		size_t convertedSize = 0;
		ntcStatus = textureSetMetadata->GetInferenceWeights(weightType, &pWeightData, &weightDataSize, &convertedSize);
		BEAR_CORE_ASSERT(ntcStatus == ntc::Status::Ok, "Filed to get the inference weights, code = {} : {}", ntc::StatusToString(ntcStatus), ntc::GetLastErrorMessage());

		ntc::StreamRange latentRange;
		ntcStatus = textureSetMetadata->GetStreamRangeForLatents(0, textureSetMetadata->GetDesc().mips, latentRange);
		BEAR_CORE_ASSERT(ntcStatus == ntc::Status::Ok, "Filed to get the stream range for latents, code = {} : {}", ntc::StatusToString(ntcStatus), ntc::GetLastErrorMessage());

		ntc::InferenceData inferenceData;
		ntcStatus = m_NtcContext->MakeInferenceData(textureSetMetadata, latentRange, weightType, &inferenceData);
		BEAR_CORE_ASSERT(ntcStatus == ntc::Status::Ok, "Filed to make the inference data, code = {} : {}", ntc::StatusToString(ntcStatus), ntc::GetLastErrorMessage());

		std::shared_ptr<NtcMaterial> material = std::make_shared<NtcMaterial>(*m_RenderContext->device);
		// create the GPU resources for inference data.
		size_t dataSize = sizeof(inferenceData.constants);
		material->SetBuffer(NtcMaterialSlot::Constant, dataSize, &inferenceData);
		dataSize = latentRange.size;
		std::vector<uint8_t> latentData(dataSize);
		inputFile->Seek(latentRange.offset);
		auto status = inputFile->Read(latentData.data(), dataSize);
		BEAR_CORE_ASSERT(status, "Failed to read latent data from input file!");
		material->SetBuffer(NtcMaterialSlot::Latent, dataSize, latentData.data());
		dataSize = convertedSize ? convertedSize : weightDataSize;
		material->SetBuffer(NtcMaterialSlot::Weight, dataSize, pWeightData);

		return material;
	}
}
