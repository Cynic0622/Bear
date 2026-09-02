#include "bearpch.h"
#include "ResourceManager.h"
#include "AssetLoader.h"
#include "Material.h"
#include "Texture.h"
#include "Mesh.h"
#include "NtcMaterial.h"
#include "PbrMaterial.h"
namespace Bear
{
	ResourceManager::ResourceManager(RHIDevice& device)
		:m_Device(device)
	{
	}
	ResourceManager::ResourceManager(RenderContext* context)
		:m_Device(*context->device)
	{
		m_RenderContext = context;
	}
	std::shared_ptr<Texture> ResourceManager::CreateTexture(const ImageDescription& imageDesc, const SamplerDescription& samplerDesc)
	{
		return std::make_shared<Texture>(m_Device, imageDesc, samplerDesc);
	}
	std::shared_ptr<Mesh> ResourceManager::CreateMesh(const PrimitiveDescription& desc)
	{
		return std::make_shared<Mesh>(m_Device, desc.vertices, desc.indices);
	}
	std::shared_ptr<Material> ResourceManager::CreateMaterial(const MaterialDescription& desc, std::unordered_map<int, std::shared_ptr<Texture>> textures)
	{
		if (m_RenderContext->useTextureCompression)
		{
			return CreateNtcMaterial(desc, textures);
		}
		else
		{
			return CreatePbrMaterial(desc, textures);
		}
	}
	Resources ResourceManager::CreateResources(const ModelDescription& desc)
	{
		Resources res{};
		res.Materials.resize(desc.materials.size());
		// be careful when using smart pointers pointing temp objects, they will be destroyed after this function returns.
		std::vector<std::shared_ptr<Texture>> textures(desc.textures.size());
		if (desc.samplers.empty())
		{
			for (size_t i = 0; i < desc.textures.size(); ++i)
			{
				textures[i] = CreateTexture(desc.images[desc.textures[i].imageIndex], SamplerDescription{});
				// use default sampler if not specified
			}
		}
		else
		{
			for (size_t i = 0; i < desc.textures.size(); ++i)
			{
				textures[i] = CreateTexture(desc.images[desc.textures[i].imageIndex],
				                            desc.samplers[desc.textures[i].samplerIndex == -1
					                                          ? 0
					                                          : desc.textures[i].samplerIndex]);
			}
		}
		for (size_t i = 0; i < desc.materials.size(); ++i)
		{
			std::unordered_map<int, std::shared_ptr<Texture>> textureMap;
			if (desc.materials[i].baseColorTextureIndex != -1)
			{
				textureMap[desc.materials[i].baseColorTextureIndex] = textures[desc.materials[i].baseColorTextureIndex];
			}
			if (desc.materials[i].metallicRoughnessTextureIndex != -1)
			{
				textureMap[desc.materials[i].metallicRoughnessTextureIndex] = textures[desc.materials[i].metallicRoughnessTextureIndex];
			}
			if (desc.materials[i].normalTextureIndex != -1)
			{
				textureMap[desc.materials[i].normalTextureIndex] = textures[desc.materials[i].normalTextureIndex];
			}
			if (desc.materials[i].occlusionTextureIndex != -1)
			{
				textureMap[desc.materials[i].occlusionTextureIndex] = textures[desc.materials[i].occlusionTextureIndex];
			}
			if (desc.materials[i].emissiveTextureIndex != -1)
			{
				textureMap[desc.materials[i].emissiveTextureIndex] = textures[desc.materials[i].emissiveTextureIndex];
			}

			res.Materials[i] = CreateMaterial(desc.materials[i], textureMap);
		}

		size_t totalVertices = 0;
		size_t totalIndices = 0;
		for (const auto& prim : desc.primitives)
		{
			totalVertices += prim.vertices.size();
			totalIndices += prim.indices.size();
		}

		res.Meshes.resize(desc.primitives.size());
		if (totalVertices > 0 && totalIndices > 0)
		{
			m_GlobalVertexBuffer = m_Device.CreateBuffer(
				totalVertices * sizeof(VertexDescription),
				BufferUsage::VertexBuffer | BufferUsage::TransferDstBuffer, true);
			m_GlobalIndexBuffer = m_Device.CreateBuffer(
				totalIndices * sizeof(uint32_t),
				BufferUsage::IndexBuffer | BufferUsage::TransferDstBuffer, true);

			size_t vtxOffset = 0;
			size_t idxOffset = 0;
			for (size_t i = 0; i < desc.primitives.size(); ++i)
			{
				const auto& prim = desc.primitives[i];
				auto mesh = std::make_shared<Mesh>(m_Device, prim.vertices, prim.indices);

				size_t vtxSize = prim.vertices.size() * sizeof(VertexDescription);
				size_t idxSize = prim.indices.size() * sizeof(uint32_t);

				if (vtxSize > 0)
					m_GlobalVertexBuffer->UploadData(prim.vertices.data(), vtxSize, vtxOffset);
				if (idxSize > 0)
					m_GlobalIndexBuffer->UploadData(prim.indices.data(), idxSize, idxOffset);

				mesh->SetupIndirect(static_cast<int32_t>(vtxOffset / sizeof(VertexDescription)),
					static_cast<uint32_t>(idxOffset / sizeof(uint32_t)));

				vtxOffset += vtxSize;
				idxOffset += idxSize;

				res.Meshes[i] = std::move(mesh);
			}

			Mesh::SetGlobalBuffers(m_GlobalVertexBuffer.get(), m_GlobalIndexBuffer.get());
		}
		else
		{
			for (size_t i = 0; i < desc.primitives.size(); ++i)
			{
				res.Meshes[i] = CreateMesh(desc.primitives[i]);
			}
		}
		return res;
	}
	std::shared_ptr<Texture> ResourceManager::GetDefaultTexture(int bindingSlot)
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

	std::shared_ptr<Material> ResourceManager::CreateNtcMaterial(const MaterialDescription& desc, const std::unordered_map<int, std::shared_ptr<Texture>>& textures) const
	{
		return std::make_shared<NtcMaterial>(*m_RenderContext->device, desc, textures);
	}
	std::shared_ptr<Material> ResourceManager::CreatePbrMaterial(const MaterialDescription& desc, const std::unordered_map<int, std::shared_ptr<Texture>>& textures)
	{
		auto material = std::make_shared<PbrMaterial>(m_Device);
		auto bindTex = [&](int slot, int textureIndex)
			{
				if (textureIndex >= 0 && textures.contains(textureIndex)) 
				{
					material->SetTexture(slot, textures.at(textureIndex)); // bind the texture to the material
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
		material->SetTransparent(desc.isTransparent);
		return material;
	}
}
