//
// Created by shw on 2025/8/5.
//
#pragma once
#include "Core/ResourceManager.h"
namespace Bear
{
    class RHIDevice;
	class RHIImage;
	class RHISampler;
    class Texture
    {
    public:
        Texture(RHIDevice& device, const std::string& path);
        Texture(RHIDevice& device, uint32_t width, uint32_t height, uint32_t channels, const void* pixels);
		Texture(RHIDevice& device, const ImageDescription& imageDesc, const SamplerDescription& samplerDesc);
        ~Texture() = default;

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

		const RHIImage& GetImage() const { return *m_Image; }
		const RHISampler& GetSampler() const { return *m_Sampler; }
		const uint32_t GetWidth() const { return m_Width; }
		const uint32_t GetHeight() const { return m_Height; }
        const uint8_t GetChannels() const { return m_Channels; }
		const std::vector<unsigned char>& GetImageData() const
		{
            if (m_ImageData.empty())
            {
                BEAR_CORE_ERROR("Invalid image data!");
            }
			return m_ImageData;
		}
		const std::string GetName() const { return m_Name; }

	private:
		std::shared_ptr<RHIImage> m_Image;
		std::shared_ptr<RHISampler> m_Sampler;
        uint8_t m_Channels;
        uint32_t m_Width;
		uint32_t m_Height;
        std::vector<unsigned char> m_ImageData;
        std::string m_Name;

    private:
        PixelFormat selectFormat(uint32_t channels);
    };
    using TextureManager = ResourceManager<Texture, std::string>;
} // Bear

