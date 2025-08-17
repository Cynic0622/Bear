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
	std::shared_ptr<Mesh> Resource::CreateMesh(const PrimitiveDescription& desc)
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
			else
			{
				material->SetTexture(slot, GetDefaultTexture(slot)); // bind default texture if not available
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
		// be careful when using smart pointers pointing temp objects, they will be destroyed after this function returns.
		std::vector<std::shared_ptr<Texture>> images(desc.images.size());
		for (size_t i = 0; i < desc.images.size(); ++i)
		{
			images[i] = CreateTexture(desc.images[i]);
		}

		for (size_t i = 0; i < desc.materials.size(); ++i)
		{
			res.Materials[i] = CreateMaterial(desc.materials[i], images);
		}

		res.Meshes.resize(desc.primitives.size());
		for (size_t i = 0; i < desc.primitives.size(); ++i)
		{
			res.Meshes[i] = CreateMesh(desc.primitives[i]);
		}
		return res;
	}
	std::shared_ptr<Texture> Resource::GetDefaultTexture(int bindingSlot)
	{
		// cache per-slot defaults to avoid reallocating
		

		if (bindingSlot >= 0 && bindingSlot < (int)m_DefaultTextures.size() && m_DefaultTextures[bindingSlot])
			return m_DefaultTextures[bindingSlot];

		uint8_t pixel[4] = { 0xFF, 0xFF, 0xFF, 0xFF }; // default white

		switch (bindingSlot)
		{
		case MaterialSlot::BaseColor:
			// white opaque
			pixel[0] = 0xFF; pixel[1] = 0xFF; pixel[2] = 0xFF; pixel[3] = 0xFF;
			break;
		case MaterialSlot::MetallicRoughness:
			// roughness = 1.0 -> G = 255, metallic = 0.0 -> B = 0
			pixel[0] = 0xFF; pixel[1] = 0xFF; pixel[2] = 0x00; pixel[3] = 0xFF;
			break;
		case MaterialSlot::Normal:
			// neutral normal = (0.5, 0.5, 1.0) -> (128,128,255)
			pixel[0] = 128; pixel[1] = 128; pixel[2] = 255; pixel[3] = 0xFF;
			break;
		case MaterialSlot::Occlusion:
			// occlusion = 1.0 -> R = 255
			pixel[0] = 0xFF; pixel[1] = 0xFF; pixel[2] = 0xFF; pixel[3] = 0xFF;
			break;
		case MaterialSlot::Emissive:
			// black (no emission)
			pixel[0] = 0x00; pixel[1] = 0x00; pixel[2] = 0x00; pixel[3] = 0xFF;
			break;
		default:
			pixel[0] = 0xFF; pixel[1] = 0xFF; pixel[2] = 0xFF; pixel[3] = 0xFF;
			break;
		}

		auto tex = std::make_shared<Texture>(m_Device, 1, 1, 4, pixel);
		if (bindingSlot >= 0 && bindingSlot < (int)m_DefaultTextures.size())
			m_DefaultTextures[bindingSlot] = tex;
		return tex;
	}
}
