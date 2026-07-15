//
// Created by shw on 2025/8/5.
//

#include "Texture.h"

#include <stb_image.h>
#include <cmath>

#include "RHI/RHIDevice.h"
#include "RHI/RHIResources.h"
#include "RHI/RHITypes.h"

namespace Bear
{
	Texture::Texture(RHIDevice& device, const std::string& path)
	{
		RHITextureConfig config;
		int32_t texWidth, texHeight, texChannels;
		std::unique_ptr<RHIBuffer> stagingBuffer;
		if (path.ends_with(".hdr"))
		{
			float* pixels = stbi_loadf(path.c_str(), &texWidth, &texHeight, &texChannels, 4);
			BEAR_CORE_ASSERT(pixels != nullptr, "Failed to load texture image: " + path);
			config.format = PixelFormat::R32G32B32A32_SFLOAT;
			size_t imageSize = texWidth * texHeight * 4 * sizeof(float);
			stagingBuffer = device.CreateBuffer(imageSize, BufferUsage::StagingBuffer, true);
			stagingBuffer->UploadData(pixels, imageSize);
			stbi_image_free(pixels);
		}
		else
		{
			unsigned char*  pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, 4);
			BEAR_CORE_ASSERT(pixels != nullptr, "Failed to load texture image: " + path);
			config.format = selectFormat(4);
			size_t imageSize = texWidth * texHeight * 4 * sizeof(*pixels);
			stagingBuffer = device.CreateBuffer(imageSize, BufferUsage::StagingBuffer, true);
			stagingBuffer->UploadData(pixels, imageSize);
			stbi_image_free(pixels);
		}

		config.width = texWidth;
		config.height = texHeight;
		uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
		config.mipLevels = mipLevels;

		m_Image = device.CreateTexture(config);
		m_Channels = 4;
		m_Width = texWidth;
		m_Height = texHeight;
		device.ImmediateSubmit([&](RHICommandList& cmd)
			{
				cmd.TransitionImageLayout(*m_Image, ImageLayout::Undefined, ImageLayout::TransferDst);

				cmd.CopyBufferToTexture(*stagingBuffer, *m_Image);

				if (mipLevels > 1)
					GenerateMips(cmd, *m_Image, mipLevels);
				else
					cmd.TransitionImageLayout(*m_Image, ImageLayout::TransferDst, ImageLayout::ShaderReadOnly);
			});

		RHISamplerConfig samplerConfig = RHISamplerConfig::GetDefault();
		m_Sampler = device.CreateSampler(samplerConfig);
	}
	Texture::Texture(RHIDevice& device, uint32_t width, uint32_t height, uint32_t channels, const void* pixels)
		:m_Channels(channels), m_Width(width), m_Height(height)
	{
		size_t imageSize = width * height * channels;

		auto stagingBuffer = device.CreateBuffer(imageSize, BufferUsage::StagingBuffer, true);
		stagingBuffer->UploadData(pixels, imageSize);

		RHITextureConfig config;
		config.width = width;
		config.height = height;
		config.format = PixelFormat::R8G8B8A8_UNORM;
		uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
		config.mipLevels = mipLevels;
		m_Image = device.CreateTexture(config);

		device.ImmediateSubmit([&](RHICommandList& cmd)
			{
				cmd.TransitionImageLayout(*m_Image, ImageLayout::Undefined, ImageLayout::TransferDst);
				cmd.CopyBufferToTexture(*stagingBuffer, *m_Image);
				if (mipLevels > 1)
					GenerateMips(cmd, *m_Image, mipLevels);
				else
					cmd.TransitionImageLayout(*m_Image, ImageLayout::TransferDst, ImageLayout::ShaderReadOnly);
			});

		m_Sampler = device.CreateSampler(RHISamplerConfig::GetDefault());
	}
	Texture::Texture(RHIDevice& device, const ImageDescription& imageDesc, const SamplerDescription& samplerDesc)
	{
		BEAR_CORE_ASSERT(!imageDesc.pixels.empty(), "Image pixels cannot be empty");
		if (imageDesc.channels != 4)
		{
			// if the image channels is not 4, we need to convert it to 4 channels for GPU resource.
			size_t pixelCount = imageDesc.width * imageDesc.height;
			m_ImageData.resize(pixelCount * 4);

			for (size_t i = 0; i < pixelCount; ++i)
			{
				for (uint8_t c = 0; c < 4; ++c)
				{
					if (c < imageDesc.channels)
					{
						m_ImageData[i * 4 + c] = imageDesc.pixels[i * imageDesc.channels + c];
					}
					else
					{
						m_ImageData[i * 4 + c] = 255; // set alpha to 255 if not present
					}
				}
			}
		}
		else
		{
			m_ImageData = imageDesc.pixels;
		}
		size_t imageSize = m_ImageData.size();
		auto stagingBuffer = device.CreateBuffer(imageSize, BufferUsage::StagingBuffer, true);
		stagingBuffer->UploadData(m_ImageData.data(), imageSize);
		RHITextureConfig config;
		config.width = imageDesc.width;
		config.height = imageDesc.height;
		uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(imageDesc.width, imageDesc.height)))) + 1;
		// if the name contains "normal" or "roughness", use UNORM format.
		config.format = (imageDesc.name.find("normal") != std::string::npos || imageDesc.name.find("Normal") != std::string::npos
			|| imageDesc.name.find("roughness") != std::string::npos || imageDesc.name.find("Roughness") != std::string::npos ||
			imageDesc.name.find("occlusion") != std::string::npos)
							? PixelFormat::R8G8B8A8_UNORM
			: PixelFormat::R8G8B8A8_SRGB;
		config.mipLevels = mipLevels;
		m_Image = device.CreateTexture(config);
		m_Channels = imageDesc.channels;
		m_Height = imageDesc.height;
		m_Width = imageDesc.width;
		m_Name = imageDesc.name;
		device.ImmediateSubmit([&](RHICommandList& cmd)
			{
				cmd.TransitionImageLayout(*m_Image, ImageLayout::Undefined, ImageLayout::TransferDst);
				cmd.CopyBufferToTexture(*stagingBuffer, *m_Image);
				if (mipLevels > 1)
					GenerateMips(cmd, *m_Image, mipLevels);
				else
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
	void Texture::GenerateMips(RHICommandList& cmd, RHIImage& image, uint32_t mipLevels)
	{
		cmd.TransitionImageLayout(image, ImageLayout::TransferDst, ImageLayout::TransferSrc, 0, 1);
		for (uint32_t i = 1; i < mipLevels; ++i)
		{
			cmd.TransitionImageLayout(image, ImageLayout::Undefined, ImageLayout::TransferDst, i, 1);
			cmd.BlitImage(image, image, i - 1, i);
			ImageLayout nextLayout = (i == mipLevels - 1) ? ImageLayout::ShaderReadOnly : ImageLayout::TransferSrc;
			cmd.TransitionImageLayout(image, ImageLayout::TransferDst, nextLayout, i, 1);
		}
		cmd.TransitionImageLayout(image, ImageLayout::TransferSrc, ImageLayout::ShaderReadOnly, 0, mipLevels - 1);
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