#include "wvk_skeleton.h"

#include <logger.h>

namespace wvk {

std::vector<MeshVertex> skeletonVertexToWvkVertex(const std::vector<RiggedMeshVertex> vertices) {
    std::vector<MeshVertex> converted;
    for (auto skeletonVertex : vertices) {
        MeshVertex vertex{skeletonVertex.position, skeletonVertex.normal, skeletonVertex.tex_coord};
        converted.push_back(vertex);
    }
    return converted;
}

WvkSkeleton::WvkSkeleton(WvkDevice& device, std::string filename) :
    device{device}, skeleton{filename} {

    createIndexBuffer();
    logger::debug("Created skeleton index buffer");

    createVertexBuffer();
    logger::debug("Created skeleton vertex buffer");
}

WvkSkeleton::~WvkSkeleton() {
    vertexBuffer.cleanup();
    vertexStagingBuffer.cleanup();
    indexBuffer.cleanup();
    indexStagingBuffer.cleanup();
}

void WvkSkeleton::createVertexBuffer() {
    // Size in bytes of buffer
    const auto &vertices = skeleton.getVertices();
    VkDeviceSize size = sizeof(vertices[0]) * vertices.size();

    // Create vertex buffer
    device.createBuffer(size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        vertexBuffer);

    // Create staging buffer
    device.createBuffer(size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        vertexStagingBuffer);

    // Copy vertices to staging buffer
    void *pData;
    vkMapMemory(device.getDevice(), vertexStagingBuffer.memory, 0, size, 0, &pData);
    memcpy(pData, vertices.data(), (size_t) size);
    vkUnmapMemory(device.getDevice(), vertexStagingBuffer.memory);

    device.copyBuffer(vertexStagingBuffer, vertexBuffer, size);
}

void WvkSkeleton::createIndexBuffer() {
    // Size in bytes of buffer
    const auto &indices = skeleton.getIndices();
    VkDeviceSize size = sizeof(indices[0]) * indices.size();

    // Create index buffer
    device.createBuffer(size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        indexBuffer);

    // Create staging buffer
    device.createBuffer(size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        indexStagingBuffer);

    // Copy indices to staging buffer
    void *pData;
    vkMapMemory(device.getDevice(), indexStagingBuffer.memory, 0, size, 0, &pData);
    memcpy(pData, indices.data(), (size_t) size);
    vkUnmapMemory(device.getDevice(), indexStagingBuffer.memory);

    device.copyBuffer(indexStagingBuffer, indexBuffer, size);
}

void WvkSkeleton::bind(VkCommandBuffer commandBuffer) {
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, &offset);

    vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, offset, VK_INDEX_TYPE_UINT32);
}

void WvkSkeleton::draw(VkCommandBuffer commandBuffer) {
    uint32_t numIndices = skeleton.getIndices().size();
    vkCmdDrawIndexed(commandBuffer, numIndices, 1, 0, 0, 0);
}

}
