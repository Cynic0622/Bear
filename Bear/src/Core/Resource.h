#pragma once
#include <string>
#include <memory>
#include <vector>

namespace Bear
{
	struct MaterialDescription;
}

namespace Bear
{
	class Material;
	struct ImageDescription;
    struct SubmeshDescription;
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

        std::shared_ptr<Texture> CreateTexture(const ImageDescription& desc);
        std::shared_ptr<Mesh> CreateMesh(const SubmeshDescription& desc);
		std::shared_ptr<Material> CreateMaterial(const MaterialDescription& desc, const std::vector<std::shared_ptr<Texture>>& images);
        Resources CreateResources(const ModelDescription& desc);
		// const std::string& GetName() const;

	private:
		std::string m_Name;
		RHIDevice& m_Device;
	};
}
