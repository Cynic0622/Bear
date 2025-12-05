#pragma once
#include <string>
#include <memory>
#include <vector>
#include "Material.h"
#include <libntc/ntc.h>

namespace Bear
{
	class Material;
	struct ImageDescription;
    struct PrimitiveDescription;
    struct ModelDescription;
    class Texture;
    class Mesh;
    class RHIDevice;
	struct MaterialDescription;
    struct Resources
    {
        std::vector<std::shared_ptr<Material>> Materials;
        std::vector<std::shared_ptr<Mesh>> Meshes;
    };
	class Resource
	{
	public:
		explicit Resource(RHIDevice& device);
		Resource(RenderContext* context);
		~Resource() = default;

        std::shared_ptr<Texture> CreateTexture(const ImageDescription& imageDesc, const SamplerDescription& samplerDesc);
        std::shared_ptr<Mesh> CreateMesh(const PrimitiveDescription& desc);
		std::shared_ptr<Material> CreateMaterial(const MaterialDescription& desc, std::unordered_map<int, std::shared_ptr<Texture>> textures);
        Resources CreateResources(const ModelDescription& desc);
		std::shared_ptr<Texture> GetDefaultTexture(int bindingSlot);
	private:
		std::shared_ptr<Material> CreateNtcMaterial(const MaterialDescription& desc, const std::unordered_map<int, std::shared_ptr<Texture>>& textures) const;
		std::shared_ptr<Material> CreatePbrMaterial(const MaterialDescription& desc, const std::unordered_map<int, std::shared_ptr<Texture>>& textures);
	private:
		std::string m_Name;
		RHIDevice& m_Device;
		RenderContext* m_RenderContext = nullptr;
		std::array<std::shared_ptr<Texture>, MaterialSlot::Count> m_DefaultTextures;
	};
}
