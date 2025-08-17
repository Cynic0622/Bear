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

        std::string directory = filepath.substr(0, filepath.find_last_of('/'));

        bool res = loader.LoadASCIIFromFile(&model, &err, &warn, filepath);

        BEAR_CORE_ASSERT(warn.empty(), "glTF WARN: " + warn);
        BEAR_CORE_ASSERT(err.empty(), "glTF ERR: " + err);
        BEAR_CORE_ASSERT(res, "Failed to load glTF file: " + filepath);


		// --- Images ---
		desc.images.reserve(model.images.size());
        for (const auto& gltfImage : model.images) {
            ImageDescription imgDesc;
            imgDesc.name = gltfImage.name;
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
        // --- Materials ---
        desc.materials.reserve(model.materials.size());
        for (const auto& gltfMaterial : model.materials) {
            MaterialDescription matDesc;
            matDesc.name = gltfMaterial.name;

            const auto& pbr = gltfMaterial.pbrMetallicRoughness;
            matDesc.baseColorFactor = glm::make_vec4(pbr.baseColorFactor.data());
            matDesc.metallicFactor = static_cast<float>(pbr.metallicFactor);
            matDesc.roughnessFactor = static_cast<float>(pbr.roughnessFactor);
			matDesc.emissiveFactor = glm::make_vec3(gltfMaterial.emissiveFactor.data());

            if (pbr.baseColorTexture.index >= 0) {
                const auto& tex = model.textures[pbr.baseColorTexture.index];
                matDesc.baseColorTextureIndex = tex.source;
            }
            if (pbr.metallicRoughnessTexture.index >= 0) {
                const auto& tex = model.textures[pbr.metallicRoughnessTexture.index];
                matDesc.metallicRoughnessTextureIndex = tex.source;
			}
            if (gltfMaterial.normalTexture.index >= 0) {
                const auto& tex = model.textures[gltfMaterial.normalTexture.index];
                matDesc.normalTextureIndex = tex.source;
			}
            if (gltfMaterial.occlusionTexture.index >= 0) {
                const auto& tex = model.textures[gltfMaterial.occlusionTexture.index];
                matDesc.occlusionTextureIndex = tex.source;
            }
            if (gltfMaterial.emissiveTexture.index >= 0) {
                const auto& tex = model.textures[gltfMaterial.emissiveTexture.index];
                matDesc.emissiveTextureIndex = tex.source;
			}

            desc.materials.push_back(std::move(matDesc));
        }

        // --- Mesh ---
		desc.primitives.reserve(model.meshes.size()); // estimate size.
        uint32_t lastPrimitiveIndex = 0;
        for (const auto& gltfMesh : model.meshes) {
            MeshDescription meshDesc{ .firstPrimitiveIndex = lastPrimitiveIndex ? lastPrimitiveIndex + 1 : lastPrimitiveIndex, .primitiveCount = gltfMesh.primitives.size() };
            for (const auto& primitive : gltfMesh.primitives) {
                PrimitiveDescription submeshDesc;
                submeshDesc.materialIndex = primitive.material;

                // --- Vertex ---
                const float* positions = nullptr;
                const float* normals = nullptr;
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
                }
                desc.primitives.push_back(std::move(submeshDesc));
            }
            desc.meshes.push_back(meshDesc);
            lastPrimitiveIndex = gltfMesh.primitives.size();
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