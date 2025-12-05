//
// Created by shw on 2025/8/5.
//

#include "Texture.h"

#include <stb_image.h>

#include "RHI/RHIDevice.h"
#include "RHI/RHIResources.h"
#include "RHI/RHITypes.h"

namespace Bear
{
	Texture::Texture(RHIDevice& device, const std::string& path)
	{
		int32_t texWidth, texHeight, texChannels;
		unsigned char* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, 4);
		BEAR_CORE_ASSERT(pixels != nullptr, "Failed to load texture image: " + path);

		size_t imageSize = texWidth * texHeight * texChannels;
		std::unique_ptr<RHIBuffer> stagingBuffer = device.CreateBuffer(imageSize, BufferUsage::StagingBuffer, true);
		stagingBuffer->UploadData(pixels, imageSize);
		stbi_image_free(pixels);

		RHITextureConfig config;
		config.width = texWidth;
		config.height = texHeight;
		config.format = selectFormat(texChannels);

		m_Image = device.CreateTexture(config);

		device.ImmediateSubmit([&](RHICommandList& cmd)
			{
				cmd.TransitionImageLayout(*m_Image, ImageLayout::Undefined, ImageLayout::TransferDst);

				cmd.CopyBufferToTexture(*stagingBuffer, *m_Image);

				cmd.TransitionImageLayout(*m_Image, ImageLayout::TransferDst, ImageLayout::ShaderReadOnly);
			});

		RHISamplerConfig samplerConfig = RHISamplerConfig::GetDefault();
		m_Sampler = device.CreateSampler(samplerConfig);
	}
	Texture::Texture(RHIDevice& device, uint32_t width, uint32_t height, uint32_t channels, const void* pixels)
	{
		size_t imageSize = width * height * channels;

		auto stagingBuffer = device.CreateBuffer(imageSize, BufferUsage::StagingBuffer, true);
		stagingBuffer->UploadData(pixels, imageSize);

		RHITextureConfig config;
		config.width = width;
		config.height = height;
		config.format = selectFormat(channels);
		m_Image = device.CreateTexture(config);

		device.ImmediateSubmit([&](RHICommandList& cmd)
			{
				cmd.TransitionImageLayout(*m_Image, ImageLayout::Undefined, ImageLayout::TransferDst);
				cmd.CopyBufferToTexture(*stagingBuffer, *m_Image);
				cmd.TransitionImageLayout(*m_Image, ImageLayout::TransferDst, ImageLayout::ShaderReadOnly);
			});

		m_Sampler = device.CreateSampler(RHISamplerConfig::GetDefault());
	}
	Texture::Texture(RHIDevice& device, const ImageDescription& imageDesc, const SamplerDescription& samplerDesc)
	{
		BEAR_CORE_ASSERT(!imageDesc.pixels.empty(), "Image pixels cannot be empty");
		size_t imageSize = imageDesc.width * imageDesc.height * imageDesc.channels;
		auto stagingBuffer = device.CreateBuffer(imageSize, BufferUsage::StagingBuffer, true);
		stagingBuffer->UploadData(imageDesc.pixels.data(), imageSize);
		RHITextureConfig config;
		config.width = imageDesc.width;
		config.height = imageDesc.height;
		config.format = selectFormat(imageDesc.channels);
		m_Image = device.CreateTexture(config);
		device.ImmediateSubmit([&](RHICommandList& cmd)
			{
				cmd.TransitionImageLayout(*m_Image, ImageLayout::Undefined, ImageLayout::TransferDst);
				cmd.CopyBufferToTexture(*stagingBuffer, *m_Image);
				cmd.TransitionImageLayout(*m_Image, ImageLayout::TransferDst, ImageLayout::ShaderReadOnly);
			});
		RHISamplerConfig samplerConfig;
		samplerConfig.magFilter = samplerDesc.magFilter == 9729 ? Filter::Linear : Filter::Nearest;
		switch (samplerDesc.minFilter)
		{
		case 9728: // NEAREST
			samplerConfig.minFilter = Filter::Nearest;
			samplerConfig.mipmapMode = MipmapMode::None;
			break;
		case 9729: // LINEAR
			samplerConfig.minFilter = Filter::Linear;
			samplerConfig.mipmapMode = MipmapMode::None;
			break;

		case 9984: // NEAREST_MIPMAP_NEAREST
			samplerConfig.minFilter = Filter::Nearest;
			samplerConfig.mipmapMode = MipmapMode::Nearest;
			break;
		case 9985: // LINEAR_MIPMAP_NEAREST
			samplerConfig.minFilter = Filter::Linear;
			samplerConfig.mipmapMode = MipmapMode::Nearest;
			break;
		case 9986: // NEAREST_MIPMAP_LINEAR
			samplerConfig.minFilter = Filter::Nearest;
			samplerConfig.mipmapMode = MipmapMode::Linear;
			break;
		case 9987: // LINEAR_MIPMAP_LINEAR
			samplerConfig.minFilter = Filter::Linear;
			samplerConfig.mipmapMode = MipmapMode::Linear;
			break;
		default:
			BEAR_CORE_WARN("Unsupported minFilter value: {}", samplerDesc.minFilter);
			samplerConfig.minFilter = Filter::Linear; // default to linear if unsupported
			samplerConfig.mipmapMode = MipmapMode::Linear; // default to linear mipmap mode
			break;
		}

		auto mapWarp = [](int wrapMode) -> SamplerAddressMode
		{
			switch (wrapMode)
			{
			case 10497: // REPEAT
				return SamplerAddressMode::Repeat;
			case 33071: // CLAMP_TO_EDGE
				return SamplerAddressMode::ClampToEdge;
			case 33648: // MIRRORED_REPEAT
				return SamplerAddressMode::MirroredRepeat;
			default:
				BEAR_CORE_WARN("Unsupported wrap mode: {}", wrapMode);
				return SamplerAddressMode::Repeat; // default to repeat if unsupported
			}
		};
		samplerConfig.addressModeU = mapWarp(samplerDesc.wrapS);
		samplerConfig.addressModeV = mapWarp(samplerDesc.wrapT);
		m_Sampler = device.CreateSampler(samplerConfig);
		// m_Sampler = device.CreateSampler(RHISamplerConfig::GetDefault());
	}
	PixelFormat Texture::selectFormat(uint32_t channels)
	{
		switch (channels)
		{
			case 1:
				return PixelFormat::R8_SRGB;
			case 2:
				return PixelFormat::R8G8_SRGB;
			case 3:
				return PixelFormat::R8G8B8_SRGB;
			case 4:
				return PixelFormat::R8G8B8A8_SRGB;
			default:
				return PixelFormat::Unknown; // Unsupported format
		}
	}
} // Bear