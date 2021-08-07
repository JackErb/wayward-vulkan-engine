#pragma once

#include "wvk_buffer.h"
#include "wvk_device.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "glm.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <vector>
#include <array>

namespace wvk {


struct Vertex {
    glm::vec3 position;
    glm::vec2 tex_coord;
    uint8_t texture_index = 1;

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();

    bool operator==(const Vertex& other) const {
        return position == other.position && tex_coord == other.tex_coord;
    }
};


class WvkModel {
public:
    WvkModel(WvkDevice& device) : device{device} {}
    WvkModel(WvkDevice& device, std::string modelFilename);
    WvkModel(WvkDevice& device, std::vector<Vertex> vertices, std::vector<uint32_t> indices);
    ~WvkModel();

    WvkModel(const WvkModel &) = delete;
    WvkModel &operator=(const WvkModel &) = delete;

    void loadModel(std::vector<Vertex> vertices, std::vector<uint32_t> indices);

    void bind(VkCommandBuffer commandBuffer);
    void draw(VkCommandBuffer commandBuffer);

    VkBuffer getVertexBuffer() { return vertexBuffer.buffer; }
    VkBuffer getIndexBuffer() { return indexBuffer.buffer; }
    const std::vector<uint32_t> &getIndices() { return indices; }

private:
    void initialize();
    void createVertexBuffer();
    void createIndexBuffer();

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    WvkDevice& device;

    Buffer vertexBuffer;
    Buffer vertexStagingBuffer;

    Buffer indexBuffer;
    Buffer indexStagingBuffer;
};

}


namespace std {

template<> struct hash<wvk::Vertex> {
    size_t operator()(wvk::Vertex const& vertex) const {
        return hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec2>()(vertex.tex_coord) << 1);
    }
};

}
