#include "bearpch.h"
#include "Resource.h"
#include "AssetLoader.h"
#include "Material.h"
#include "RHI/RHIDevice.h"
#include "Texture.h"
#include "Mesh.h"
#include "PbrMaterial.h"

namespace Bear
{
	Resource::Resource(RHIDevice& device)
		:m_Device(device)
	{
	}
	std::shared_ptr<Texture> Resource::CreateTexture(const ImageDescription& desc)
	{
		return std::make_shared<Texture>(m_Device, desc.width, desc.height, desc.channels, desc.pixels.data());
	}
	std::shared_ptr<Mesh> Resource::CreateMesh(const SubmeshDescription& desc)
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
		return material;
	}
	Resources Resource::CreateResources(const ModelDescription& desc)
	{
		Resources res{};
		res.Materials.resize(desc.materials.size());
		std::vector<std::shared_ptr<Texture>> images(desc.images.size());
		for (size_t i = 0; i < desc.images.size(); ++i) {
			images[i] = CreateTexture(desc.images[i]);
		}

		for (size_t i = 0; i < desc.materials.size(); ++i) {
			res.Materials[i] = CreateMaterial(desc.materials[i], images);
		}

		res.Meshes.resize(desc.submeshes.size());
		for (size_t i = 0; i < desc.submeshes.size(); ++i) {
			res.Meshes[i] = CreateMesh(desc.submeshes[i]);
		}
		return res;
	}
}
