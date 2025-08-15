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