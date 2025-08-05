//
// Created by shw on 2025/8/5.
//

#pragma once

namespace Bear
{
    class RHIDevice;
	class RHITexture;
	class RHISampler;
    class Texture
    {
    public:
        Texture(const RHIDevice& device, const std::string& path);
        ~Texture();

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        /*RHITexture**/
	private:
		std::shared_ptr<RHITexture> m_Image;
		std::shared_ptr<RHISampler> m_Sampler;
    };
} // Bear

