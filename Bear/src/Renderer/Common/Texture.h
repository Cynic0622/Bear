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
        ~Texture() = default;

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

		const RHIImage& GetImage() const { return *m_Image; }
		const RHISampler& GetSampler() const { return *m_Sampler; }

	private:
		std::shared_ptr<RHIImage> m_Image;
		std::shared_ptr<RHISampler> m_Sampler;
    };
    using TextureManager = ResourceManager<Texture, std::string>;
} // Bear

