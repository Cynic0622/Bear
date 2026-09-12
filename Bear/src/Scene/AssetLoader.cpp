#include "bearpch.h"

#include "AssetLoader.h"

#define TINYGLTF_IMPLEMENTATION
//#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

namespace Bear {

    ModelDescription AssetLoader::ImportModel(const std::string& filepath) {
        ModelDescription desc;
        tinygltf::Model model;
        tinygltf::TinyGLTF loader;
        std::string err, warn;
        loader.SetPreserveImageChannels(true);
        std::string directory = filepath.substr(0, filepath.find_last_of('/'));

        bool res = loader.LoadASCIIFromFile(&model, &err, &warn, filepath);

        BEAR_CORE_ASSERT(warn.empty(), "glTF WARN: " + warn);
        BEAR_CORE_ASSERT(err.empty(), "glTF ERR: " + err);
        BEAR_CORE_ASSERT(res, "Failed to load glTF file: " + filepath);


		// --- Images ---
		desc.images.reserve(model.images.size());
        for (size_t i = 0; i < model.images.size(); i++) 
        {
			const auto& gltfImage = model.images[i];
            ImageDescription imgDesc;
            if (!gltfImage.uri.empty())
            {
				size_t pos = gltfImage.uri.find_last_of("/\\");
				imgDesc.name = (pos == std::string::npos) ? gltfImage.uri : gltfImage.uri.substr(pos + 1);
            }
            else if (!gltfImage.name.empty())
            {
                imgDesc.name = gltfImage.name;
            }
            else
            {
				imgDesc.name = "image_" + std::to_string(i);
            }
            imgDesc.filepath = directory + "/" + gltfImage.uri;
            
            if (!gltfImage.image.empty()) {
                imgDesc.pixels = gltfImage.image; // raw pixel data
                imgDesc.width = gltfImage.width;
                imgDesc.height = gltfImage.height;
                imgDesc.channels = gltfImage.component;
            } else {
                BEAR_CORE_ERROR("Failed to read the image form " + imgDesc.filepath);
            }
            desc.images.push_back(std::move(imgDesc));
		}
		// --- Samplers ---
		desc.samplers.reserve(model.samplers.size());
        for (const auto& gltfSampler : model.samplers) {
            SamplerDescription sampler;
            sampler.minFilter = gltfSampler.minFilter;
            sampler.magFilter = gltfSampler.magFilter;
            sampler.wrapS = gltfSampler.wrapS;
            sampler.wrapT = gltfSampler.wrapT;
            desc.samplers.push_back(sampler);
		}

		// --- Textures ---
		desc.textures.reserve(model.textures.size());
        for (const auto& gltfTexture : model.textures) {
            TextureDescription texDesc;

			size_t pos = desc.images[gltfTexture.source].name.rfind('.');
			std::string name = (pos == std::string::npos) ? desc.images[gltfTexture.source].name : desc.images[gltfTexture.source].name.substr(0, pos);
			texDesc.name = gltfTexture.name.empty() ? name : gltfTexture.name; // use the image name as the texture name if the texture name is empty.
        	if (gltfTexture.source >= 0 && static_cast<size_t>(gltfTexture.source) < model.images.size()) {
                texDesc.imageIndex = gltfTexture.source; // index in the images array
            }
            if (gltfTexture.sampler >= 0 && static_cast<size_t>(gltfTexture.sampler) < model.samplers.size()) {
                texDesc.samplerIndex = gltfTexture.sampler; // index in the samplers array
            }
            desc.textures.push_back(texDesc);
		}

        // --- Materials ---
        desc.materials.reserve(model.materials.size());
        for (const auto& gltfMaterial : model.materials) {
            MaterialDescription matDesc;
            matDesc.name = gltfMaterial.name;
            matDesc.isTransparent = gltfMaterial.alphaMode == "BLEND";
            const auto& pbr = gltfMaterial.pbrMetallicRoughness;
            matDesc.baseColorFactor = glm::make_vec4(pbr.baseColorFactor.data());
            matDesc.metallicFactor = static_cast<float>(pbr.metallicFactor);
            matDesc.roughnessFactor = static_cast<float>(pbr.roughnessFactor);
			matDesc.emissiveFactor = glm::make_vec3(gltfMaterial.emissiveFactor.data());
			
            if (pbr.baseColorTexture.index >= 0) {
                // const auto& tex = model.textures[pbr.baseColorTexture.index];
                // matDesc.baseColorTextureIndex = tex.source;
				matDesc.baseColorTextureIndex = pbr.baseColorTexture.index; // index in the textures array
				// add the semantic info at the end of the texture name, like _albedo, _diffuse, etc.
				auto pos = desc.images[desc.textures[matDesc.baseColorTextureIndex].imageIndex].name.rfind('.');
				std::string format = desc.images[desc.textures[matDesc.baseColorTextureIndex].imageIndex].name.substr(pos);
				std::string name = (pos == std::string::npos) ? desc.images[desc.textures[matDesc.baseColorTextureIndex].imageIndex].name : desc.images[desc.textures[matDesc.baseColorTextureIndex].imageIndex].name.substr(0, pos);
                desc.images[desc.textures[matDesc.baseColorTextureIndex].imageIndex].name = name + "_albedo";
                desc.images[desc.textures[matDesc.baseColorTextureIndex].imageIndex].name += format;
            }
            if (pbr.metallicRoughnessTexture.index >= 0) {
                matDesc.metallicRoughnessTextureIndex = pbr.metallicRoughnessTexture.index;
                auto pos = desc.images[desc.textures[matDesc.metallicRoughnessTextureIndex].imageIndex].name.rfind('.');
                std::string format = desc.images[desc.textures[matDesc.metallicRoughnessTextureIndex].imageIndex].name.substr(pos);
                int nameSrcIdx = (matDesc.baseColorTextureIndex >= 0) ? matDesc.baseColorTextureIndex : matDesc.metallicRoughnessTextureIndex;
                std::string name = (pos == std::string::npos) ? desc.images[desc.textures[nameSrcIdx].imageIndex].name : desc.images[desc.textures[nameSrcIdx].imageIndex].name.substr(0, pos);
                desc.images[desc.textures[matDesc.metallicRoughnessTextureIndex].imageIndex].name = name + "_roughness";
                desc.images[desc.textures[matDesc.metallicRoughnessTextureIndex].imageIndex].name += format;
            }
            if (gltfMaterial.normalTexture.index >= 0) {
                matDesc.normalTextureIndex = gltfMaterial.normalTexture.index;
                auto pos = desc.images[desc.textures[matDesc.normalTextureIndex].imageIndex].name.rfind('.');
                std::string format = desc.images[desc.textures[matDesc.normalTextureIndex].imageIndex].name.substr(pos);
                int nameSrcIdx = (matDesc.baseColorTextureIndex >= 0) ? matDesc.baseColorTextureIndex : matDesc.normalTextureIndex;
                std::string name = (pos == std::string::npos) ? desc.images[desc.textures[nameSrcIdx].imageIndex].name : desc.images[desc.textures[nameSrcIdx].imageIndex].name.substr(0, pos);
                desc.images[desc.textures[matDesc.normalTextureIndex].imageIndex].name = name + "_normal";
                desc.images[desc.textures[matDesc.normalTextureIndex].imageIndex].name += format;
            }
            if (gltfMaterial.occlusionTexture.index >= 0) {
                matDesc.occlusionTextureIndex = gltfMaterial.occlusionTexture.index;
                auto pos = desc.images[desc.textures[matDesc.occlusionTextureIndex].imageIndex].name.rfind('.');
                std::string format = desc.images[desc.textures[matDesc.occlusionTextureIndex].imageIndex].name.substr(pos);
                int nameSrcIdx = (matDesc.baseColorTextureIndex >= 0) ? matDesc.baseColorTextureIndex : matDesc.occlusionTextureIndex;
                std::string name = (pos == std::string::npos) ? desc.images[desc.textures[nameSrcIdx].imageIndex].name : desc.images[desc.textures[nameSrcIdx].imageIndex].name.substr(0, pos);
                desc.images[desc.textures[matDesc.occlusionTextureIndex].imageIndex].name = name + "_occlusion";
                desc.images[desc.textures[matDesc.occlusionTextureIndex].imageIndex].name += format;
            }
            if (gltfMaterial.emissiveTexture.index >= 0) {
                // const auto& tex = model.textures[gltfMaterial.emissiveTexture.index];
                // matDesc.emissiveTextureIndex = tex.source;
				matDesc.emissiveTextureIndex = gltfMaterial.emissiveTexture.index;
			}

            desc.materials.push_back(std::move(matDesc));
        }

        // --- Mesh ---
		desc.primitives.reserve(model.meshes.size()); // estimate size.
        uint32_t lastPrimitiveIndex = 0;
        for (const auto& gltfMesh : model.meshes) {
            MeshDescription meshDesc{ .firstPrimitiveIndex = lastPrimitiveIndex, .primitiveCount = gltfMesh.primitives.size() };
            for (const auto& primitive : gltfMesh.primitives) {
                PrimitiveDescription submeshDesc;
                submeshDesc.materialIndex = primitive.material;

                // --- Vertex ---
                const float* positions = nullptr;
                const float* normals = nullptr;
                const float* tangents = nullptr;
                const float* texCoords = nullptr;
                size_t vertexCount = 0;

                // positions
                if (primitive.attributes.contains("POSITION"))
                {
                    const auto& posAccessor = model.accessors[primitive.attributes.at("POSITION")];
                    vertexCount = posAccessor.count;
                    const auto& posBufferView = model.bufferViews[posAccessor.bufferView];
                    positions = reinterpret_cast<const float*>(&(model.buffers[posBufferView.buffer].data[posBufferView.byteOffset + posAccessor.byteOffset]));
                }

                // normals
                if (primitive.attributes.contains("NORMAL"))
                {
                    const auto& normAccessor = model.accessors[primitive.attributes.at("NORMAL")];
                    const auto& normBufferView = model.bufferViews[normAccessor.bufferView];
                    normals = reinterpret_cast<const float*>(&(model.buffers[normBufferView.buffer].data[normBufferView.byteOffset + normAccessor.byteOffset]));
                }

                // tangent
                if (primitive.attributes.contains("TANGENT"))
                {
                    const auto& tangentAccessor = model.accessors[primitive.attributes.at("TANGENT")];
                    const auto& tangentBufferView = model.bufferViews[tangentAccessor.bufferView];
                    tangents = reinterpret_cast<const float*>(&(model.buffers[tangentBufferView.buffer].data[tangentBufferView.byteOffset + tangentAccessor.byteOffset]));
                }

				// texCoords
                if (primitive.attributes.contains("TEXCOORD_0"))
                {
                    const auto& uvAccessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
                    const auto& uvBufferView = model.bufferViews[uvAccessor.bufferView];
                    texCoords = reinterpret_cast<const float*>(&(model.buffers[uvBufferView.buffer].data[uvBufferView.byteOffset + uvAccessor.byteOffset]));
                }

                submeshDesc.vertices.resize(vertexCount);
                for (size_t i = 0; i < vertexCount; ++i)
                {
                    submeshDesc.vertices[i].position = glm::make_vec3(positions + i * 3);
                    if (normals) submeshDesc.vertices[i].normal = glm::make_vec3(normals + i * 3);
                    if (texCoords) submeshDesc.vertices[i].texCoord = glm::make_vec2(texCoords + i * 2);
                    if (tangents)
                    {
                        submeshDesc.vertices[i].tangent = glm::make_vec4(tangents + i * 3);
                    }
                }

                // --- Index ---
                if (primitive.indices >= 0) {
                    const auto& indexAccessor = model.accessors[primitive.indices];
                    const auto& indexBufferView = model.bufferViews[indexAccessor.bufferView];
                    const auto& indexBuffer = model.buffers[indexBufferView.buffer];

                    submeshDesc.indices.resize(indexAccessor.count);
                    const void* dataPtr = &(indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset]);

					switch (indexAccessor.componentType)
					{
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
					{
						const uint8_t* buf = static_cast<const uint8_t*>(dataPtr);
						for (size_t i = 0; i < indexAccessor.count; ++i) submeshDesc.indices[i] = buf[i];
						break;
					}
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
					{
						const uint16_t* buf = static_cast<const uint16_t*>(dataPtr);
						for (size_t i = 0; i < indexAccessor.count; ++i) submeshDesc.indices[i] = buf[i];
						break;
					}
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
					{
						const uint32_t* buf = static_cast<const uint32_t*>(dataPtr);
						memcpy(submeshDesc.indices.data(), buf, indexAccessor.count * sizeof(uint32_t));
						break;
					}
					default:
					{
                        BEAR_CORE_ERROR("Unsupported type.");
					}
					}

                    if (!tangents)
                    {
                        std::vector<glm::vec3> tempBitangents(submeshDesc.vertices.size(), glm::vec3(0.0f));
                        for (uint32_t i = 0; i < submeshDesc.indices.size(); i += 3)
                        {
                            VertexDescription& v0 = submeshDesc.vertices[submeshDesc.indices[i]];
                            VertexDescription& v1 = submeshDesc.vertices[submeshDesc.indices[i + 1]];
                            VertexDescription& v2 = submeshDesc.vertices[submeshDesc.indices[i + 2]];
                            glm::vec3 edge1 = v1.position - v0.position;
                            glm::vec3 edge2 = v2.position - v0.position;
                            glm::vec2 deltaUV1 = v1.texCoord - v0.texCoord;
                            glm::vec2 deltaUV2 = v2.texCoord - v0.texCoord;
                            float denom = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;
                            if (std::abs(denom) < 1e-7f) continue;
                            float f = 1.0f / denom;
                            glm::vec4 tangent{0.f};
                            glm::vec3 bitangent{ 0.f };
                            tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
                            tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
                            tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

                            bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
                            bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
                            bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
                            
                            v0.tangent += tangent;
                            v1.tangent += tangent;
                            v2.tangent += tangent;
							tempBitangents[submeshDesc.indices[i]] += bitangent;
							tempBitangents[submeshDesc.indices[i + 1]] += bitangent;
							tempBitangents[submeshDesc.indices[i + 2]] += bitangent;
						}
                        for (uint32_t i = 0; i < submeshDesc.vertices.size(); ++i)
                        {
                            auto n = submeshDesc.vertices[i].normal;
                            auto t = submeshDesc.vertices[i].tangent;
                            auto b = tempBitangents[i];
                            glm::vec3 tangentXYZ = glm::vec3(t);
                            if (glm::length(tangentXYZ) < 1e-6f)
                            {
                                glm::vec3 defaultT = (std::abs(n.x) < 0.999f) ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
                                tangentXYZ = glm::normalize(defaultT - n * glm::dot(n, defaultT));
                            }
                            else
                            {
                                tangentXYZ = glm::normalize(tangentXYZ - n * glm::dot(n, tangentXYZ));
                            }
                            submeshDesc.vertices[i].tangent = glm::vec4(tangentXYZ, 0.f);
                            submeshDesc.vertices[i].tangent.w = (glm::dot(glm::cross(n, glm::vec3(t)), b) < 0.0f) ? -1.0f : 1.0f;
                        }
                    }
                }
                desc.primitives.push_back(std::move(submeshDesc));
            }
            desc.meshes.push_back(meshDesc);
            lastPrimitiveIndex += gltfMesh.primitives.size();
        }

        // --- (Nodes) ---
        desc.nodes.resize(model.nodes.size());
        for (size_t i = 0; i < model.nodes.size(); ++i) {
            const auto& gltfNode = model.nodes[i];
            auto& descNode = desc.nodes[i];

            descNode.name = gltfNode.name;
            descNode.meshIndex = gltfNode.mesh;

            if (!gltfNode.translation.empty()) descNode.translation = glm::make_vec3(gltfNode.translation.data());
            if (!gltfNode.rotation.empty()) descNode.rotation = glm::make_quat(gltfNode.rotation.data());
            if (!gltfNode.scale.empty()) descNode.scale = glm::make_vec3(gltfNode.scale.data());

            for (int childIndex : gltfNode.children) {
                descNode.childrenIndices.push_back(childIndex);
            }
        }

        // --- ensure the root node. ---
        const auto& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];
        desc.rootNodeIndices.assign(scene.nodes.begin(), scene.nodes.end());

        return desc;
    }
} // namespace Bear