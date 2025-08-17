#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Bear {

	struct VertexDescription
	{
		glm::vec3 position{ 0.0f };
		glm::vec3 normal{ 0.0f };
		glm::vec2 texCoord{ 0.0f };
	};

    struct ImageDescription {
        std::string name;
		std::string filepath; // generate the key from this
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t channels = 0;
		std::vector<unsigned char> pixels; // raw pixel data
    };

    struct SamplerDescription
    {
        int minFilter;
		int magFilter;
		int wrapS; // REPEAT, CLAMP_TO_EDGE, etc.
		int wrapT; // REPEAT, CLAMP_TO_EDGE, etc.
    };

    struct TextureDescription
    {
		int imageIndex = -1; // index in the images array
		int samplerIndex = -1; // index in the samplers array
		std::string name; // texture name, can be used as a key
    };
    struct MaterialDescription {
        std::string name;

        // PBR Metallic-Roughness Workflow
        glm::vec4 baseColorFactor{ 1.0f };
        int baseColorTextureIndex = -1;

        float metallicFactor = 1.0f;
        float roughnessFactor = 1.0f;
        int metallicRoughnessTextureIndex = -1;

        int normalTextureIndex = -1;
        int occlusionTextureIndex = -1;
        int emissiveTextureIndex = -1;
        glm::vec3 emissiveFactor{ 0.0f };
    };

    struct PrimitiveDescription {
        // mesh
        std::vector<VertexDescription> vertices;
        std::vector<uint32_t> indices;

        // material index
        int materialIndex = -1;
    };

    struct MeshDescription
    {
        uint32_t firstPrimitiveIndex = 0;
        size_t primitiveCount = 0;
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

	// the model description, contains all the primitives, materials and nodes.
    struct ModelDescription {
        std::vector<PrimitiveDescription> primitives;
		std::vector<ImageDescription> images; // images, used for textures
		std::vector<TextureDescription> textures; // textures
		std::vector<SamplerDescription> samplers; // texture samplers
        std::vector<MaterialDescription> materials;
        std::vector<NodeDescription> nodes;
        std::vector<MeshDescription> meshes;
        std::vector<uint32_t> rootNodeIndices;
    };

	// analyze and import asset files, currently only supports glTF format.
    class BEAR_API AssetLoader {
    public:
        static ModelDescription ImportModel(const std::string& filepath);
    };

} // namespace Bear