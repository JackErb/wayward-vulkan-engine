#pragma once

#include "glm.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <vector>
#include <array>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace wvk {

struct MeshVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 tex_coord;
    uint8_t texture_index;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};

        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(MeshVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{4};

        /* Vertex position */
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(MeshVertex, position);

        /* Vertex normal */
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(MeshVertex, normal);

        /* Texture coordinate */
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(MeshVertex, tex_coord);

        /* Texture id */
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].format = VK_FORMAT_R8_UINT;
        attributeDescriptions[3].offset = offsetof(MeshVertex, texture_index);

        return attributeDescriptions;
    }

    bool operator==(const MeshVertex& other) const {
        return position == other.position && tex_coord == other.tex_coord && normal == other.normal && texture_index == other.texture_index;
    }
};

struct RiggedMeshVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 tex_coord;
    uint8_t texture_index;
    uint8_t joint1;
    float weight1;
    uint8_t joint2;
    float weight2;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};

        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(RiggedMeshVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{8};

        /* Vertex position */
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(RiggedMeshVertex, position);

        /* Vertex normal */
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(RiggedMeshVertex, normal);

        /* Texture coordinate */
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(RiggedMeshVertex, tex_coord);

        /* Texture id */
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].format = VK_FORMAT_R8_UINT;
        attributeDescriptions[3].offset = offsetof(RiggedMeshVertex, texture_index);

        attributeDescriptions[4].location = 4;
        attributeDescriptions[4].binding = 0;
        attributeDescriptions[4].format = VK_FORMAT_R8_UINT;
        attributeDescriptions[4].offset = offsetof(RiggedMeshVertex, joint1);

        attributeDescriptions[5].location = 5;
        attributeDescriptions[5].binding = 0;
        attributeDescriptions[5].format = VK_FORMAT_R8_UINT;
        attributeDescriptions[5].offset = offsetof(RiggedMeshVertex, joint2);

        attributeDescriptions[6].location = 6;
        attributeDescriptions[6].binding = 0;
        attributeDescriptions[6].format = VK_FORMAT_R32_SFLOAT;
        attributeDescriptions[6].offset = offsetof(RiggedMeshVertex, weight1);

        attributeDescriptions[7].location = 7;
        attributeDescriptions[7].binding = 0;
        attributeDescriptions[7].format = VK_FORMAT_R32_SFLOAT;
        attributeDescriptions[7].offset = offsetof(RiggedMeshVertex, weight2);

        return attributeDescriptions;
    }

    bool operator==(const RiggedMeshVertex& other) const {
        return position == other.position && tex_coord == other.tex_coord && normal == other.normal && texture_index == other.texture_index
            && joint1 == other.joint1 && joint2 == other.joint2 && weight1 == other.weight1 && weight2 == other.weight2;
    }
};

}


namespace std {

template<> struct hash<wvk::MeshVertex> {
    size_t operator()(wvk::MeshVertex const& vertex) const {
        return hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.normal) << 1);
    }
};

template<> struct hash<wvk::RiggedMeshVertex> {
    size_t operator()(wvk::MeshVertex const& vertex) const {
        return hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.normal) << 1);
    }
};

}
