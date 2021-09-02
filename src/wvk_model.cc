#include "wvk_model.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "resource_path.h"

#include <unordered_map>

namespace wvk {

WvkModel::WvkModel(WvkDevice& device, std::string modelFilename, int textureId) : device{device} {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    std::string filepath = resourcePath() + modelFilename;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str())) {
        throw std::runtime_error(warn + "\n" + err);
    }

    if (warn.size() > 0) {
        logger::debug(warn);
    }

    std::unordered_map<MeshVertex, uint32_t> indexMap{};

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            MeshVertex vertex{};

            vertex.position = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.tex_coord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.texture_index = textureId;

            vertex.normal = {
                attrib.normals[3 * index.normal_index + 0],
                attrib.normals[3 * index.normal_index + 1],
                attrib.normals[3 * index.normal_index + 2]
            };

            if (indexMap.count(vertex) == 0) {
                uint32_t index = static_cast<uint32_t>(vertices.size());
                indexMap[vertex] = index;
                vertices.push_back(vertex);
            }

            indices.push_back(indexMap[vertex]);
        }
    }

    initialize();
}

WvkModel::WvkModel(WvkDevice& device, std::vector<MeshVertex> vertices, std::vector<uint32_t> indices)
                   : device{device}, vertices{vertices}, indices{indices} {
    initialize();
}

void WvkModel::loadModel(std::vector<MeshVertex> vertices, std::vector<uint32_t> indices) {
    this->vertices = vertices;
    this->indices = indices;

    initialize();
}

void WvkModel::initialize() {
    createVertexBuffer();
    logger::debug("Created vertex buffer");
    createIndexBuffer();
    logger::debug("Created index buffer");
}

WvkModel::~WvkModel() {
    vertexBuffer.cleanup();
    vertexStagingBuffer.cleanup();
    indexBuffer.cleanup();
    indexStagingBuffer.cleanup();
}

void WvkModel::createVertexBuffer() {
    // Size in bytes of buffer
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

void WvkModel::createIndexBuffer() {
    // Size in bytes of buffer
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

void WvkModel::bind(VkCommandBuffer commandBuffer) {
    std::array<VkBuffer, 1> buffers = {vertexBuffer.buffer};
    std::array<VkDeviceSize, 1> offsets  = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, buffers.size(), buffers.data(), offsets.data());
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
}

void WvkModel::draw(VkCommandBuffer commandBuffer) {
    vkCmdDrawIndexed(commandBuffer, indices.size(), 1, 0, 0, 0);
}

}
