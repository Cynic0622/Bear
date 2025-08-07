#pragma once

#include <optional>
#include <glm/glm.hpp>

namespace Bear {
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        // std::optional<uint32_t> transferFamily; // 未来可以添加专门的传输队列

        bool IsComplete() const {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct Vertex {
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 texCoord; // 添加纹理坐标属性

        // 绑定描述：告诉 Vulkan 如何将数据打包到内存中
        static VkVertexInputBindingDescription GetBindingDescription() {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0; // 顶点缓冲区在哪个绑定点
            bindingDescription.stride = sizeof(Vertex); // 每个顶点的大小
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            return bindingDescription;
        }

        // 属性描述：告诉 Vulkan 每个顶点属性的细节
        static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions() {
            std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);

            // positon (location = 0 in shader)
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // 3个32位浮点数
            attributeDescriptions[0].offset = offsetof(Vertex, pos);

            // normal (location = 1 in shader)
            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // 3个32位浮点数
            attributeDescriptions[1].offset = offsetof(Vertex, normal);

			// texcoord (location = 2 in shader)
			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT; // 2个32位浮点数
			attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

            return attributeDescriptions;
        }

        bool operator==(const Vertex& other) const {
			return pos == other.pos && normal == other.normal && texCoord == other.texCoord;
        }
    };

    struct UniformBufferObject {
        glm::mat4 view;
        glm::mat4 proj;
    };

    struct PerObjectPushConstants {
        glm::mat4 model;
        //glm::vec3 lightPos; // 光源位置
        //float padding; // 确保结构体大小为16的倍数
	};
}