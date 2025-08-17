#pragma once
#include <string>
#include <memory>
#include <vector>

#include "Material.h"

namespace Bear
{
	struct MaterialDescription;
}

namespace Bear
{
	class Material;
	struct ImageDescription;
    struct PrimitiveDescription;
    struct ModelDescription;
    class Texture;
    class Mesh;
    class RHIDevice;
    struct Resources
    {
        std::vector<std::shared_ptr<Material>> Materials;
        std::vector<std::shared_ptr<Mesh>> Meshes;
    };
	class Resource
	{
	public:
		explicit Resource(RHIDevice& device);
		~Resource() = default;

        std::shared_ptr<Texture> CreateTexture(const ImageDescription& imageDesc, const SamplerDescription& samplerDesc);
        std::shared_ptr<Mesh> CreateMesh(const PrimitiveDescription& desc);
		std::shared_ptr<Material> CreateMaterial(const MaterialDescription& desc, const std::vector<std::shared_ptr<Texture>>& images);
        Resources CreateResources(const ModelDescription& desc);
		// const std::string& GetName() const;
		std::shared_ptr<Texture> GetDefaultTexture(int bindingSlot);

	private:
		std::string m_Name;
		RHIDevice& m_Device;
		std::array<std::shared_ptr<Texture>, MaterialSlot::Count> m_DefaultTextures;
	};
}
