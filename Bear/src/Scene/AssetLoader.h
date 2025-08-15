#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Bear {

    struct ImageDescription {
        std::string name;
		std::string filepath; // generate the key from this
        int width = 0;
        int height = 0;
        int channels = 0;
		std::vector<unsigned char> pixels; // raw pixel data
    };
    struct MaterialDescription {
        std::string name;

        // PBR Metallic-Roughness Workflow
        glm::vec4 baseColorFactor{ 1.0f };
        uint32_t baseColorTextureIndex = -1;

        float metallicFactor = 1.0f;
        float roughnessFactor = 1.0f;
        uint32_t metallicRoughnessTextureIndex = -1;

        uint32_t normalTextureIndex = -1;
        uint32_t occlusionTextureIndex = -1;
        uint32_t emissiveTextureIndex = -1;
        glm::vec3 emissiveFactor{ 0.0f };
    };

    struct SubmeshDescription {
        // mesh
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        // material index
        uint32_t materialIndex = 0;
    };

	// node description, used for scene graph hierarchy
    struct NodeDescription {
        std::string name;
        glm::vec3 translation{ 0.0f };
        glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
        glm::vec3 scale{ 1.0f };

		// the index of the mesh in the mesh array, -1 if no mesh is associated
        int32_t meshIndex = -1;

		// hierarchy information
		int32_t parentIndex = -1; // the index of the parent array, -1 if no parent
        std::vector<uint32_t> childrenIndices;
    };

	// the model description, contains all the submeshes, materials and nodes.
    struct ModelDescription {
        std::vector<SubmeshDescription> submeshes;
		std::vector<ImageDescription> images; // textures
        std::vector<MaterialDescription> materials;
        std::vector<NodeDescription> nodes;
        std::vector<uint32_t> rootNodeIndices;
    };

	// analyze and import asset files, currently only supports glTF format.
    class BEAR_API AssetLoader {
    public:
        static ModelDescription ImportModel(const std::string& filepath);
    };

} // namespace Bear