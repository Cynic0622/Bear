#pragma once
#include <glm/vec3.hpp>

#include "Types.h"
#include "tiny_obj_loader.h"
//#define TINYGLTF_IMPLEMENTATION
//#define STB_IMAGE_IMPLEMENTATION
//#define STB_IMAGE_WRITE_IMPLEMENTATION
//#include <tiny_gltf.h>
// hash function for Vertex
namespace std
{
	template <>
	struct hash<glm::vec3>
	{
		size_t operator()(const glm::vec3& v) const
		{
			size_t h1 = hash<float>()(v.x);
			size_t h2 = hash<float>()(v.y);
			size_t h3 = hash<float>()(v.z);
			size_t seed = h1;
			seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
		}
	};

	template <>
	struct hash<glm::vec2>
	{
		size_t operator()(const glm::vec2& v) const
		{
			size_t h1 = hash<float>()(v.x);
			size_t h2 = hash<float>()(v.y);
			size_t seed = h1;
			seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
		}
	};
	template <>
	struct hash<Bear::Vertex>
	{
		size_t operator()(const Bear::Vertex& vertex) const
		{
			size_t h1 = hash<glm::vec3>()(vertex.pos);
			size_t h2 = hash<glm::vec3>()(vertex.normal);
			size_t h3 = hash<glm::vec2>()(vertex.texCoord);

			size_t seed = h1;
			seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
		}
	};
}
namespace Bear
{
	struct ModelData
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
	};

	inline ModelData LoadModelDataFromFile(const std::string& path)
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string err, warn;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())) {
			BEAR_CORE_ERROR("Failed to load model : " + err);
		}

		ModelData modelData;
		std::unordered_map<Vertex, uint32_t> uniqueVertices;

		for (const auto& shape : shapes)
		{
			for (const auto& index : shape.mesh.indices)
			{
				Vertex vertex = {};
				vertex.pos = glm::vec3(attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]);
				vertex.normal = glm::vec3(attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]);
				if (index.texcoord_index >= 0) {
					vertex.texCoord = glm::vec2(attrib.texcoords[2 * index.texcoord_index + 0],
						attrib.texcoords[2 * index.texcoord_index + 1]);
				}
				else {
					vertex.texCoord = glm::vec2(0.0f, 0.0f); // default texture coordinate
				}
				if (!uniqueVertices.contains(vertex)) {
					uniqueVertices[vertex] = static_cast<uint32_t>(modelData.vertices.size());
					modelData.vertices.push_back(vertex);
				}
				// modelData.vertices.push_back(vertex);
				modelData.indices.push_back(uniqueVertices[vertex]);
			}
		}

#ifdef BEAR_DEBUG
		BEAR_CORE_INFO("Model loaded: {} | Vertices: {} | Indices: {}", path, modelData.vertices.size(), modelData.indices.size());
#endif
		return modelData;
	}
}
