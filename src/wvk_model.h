#pragma once

#include "wvk_buffer.h"
#include "wvk_device.h"

#include "wvk_vertex_attributes.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "glm.h"

#include <vector>
#include <array>

namespace wvk {

class WvkModel {
public:
    WvkModel(WvkDevice& device) : device{device} {}
    WvkModel(WvkDevice& device, std::string modelFilename, int textureId);
    WvkModel(WvkDevice& device, std::vector<MeshVertex> vertices, std::vector<uint32_t> indices);
    ~WvkModel();

    WvkModel(const WvkModel &) = delete;
    WvkModel &operator=(const WvkModel &) = delete;

    void loadModel(std::vector<MeshVertex> vertices, std::vector<uint32_t> indices);

    void bind(VkCommandBuffer commandBuffer);
    void draw(VkCommandBuffer commandBuffer);

    VkBuffer getVertexBuffer() { return vertexBuffer.buffer; }
    VkBuffer getIndexBuffer() { return indexBuffer.buffer; }
    const std::vector<uint32_t> &getIndices() { return indices; }

private:
    void initialize();
    void createVertexBuffer();
    void createIndexBuffer();

    std::vector<MeshVertex> vertices;
    std::vector<uint32_t> indices;

    WvkDevice& device;

    Buffer vertexBuffer;
    Buffer vertexStagingBuffer;

    Buffer indexBuffer;
    Buffer indexStagingBuffer;
};

}
