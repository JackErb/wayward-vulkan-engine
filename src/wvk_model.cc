#include "wvk_model.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "resource_path.h"

#include <unordered_map>

namespace wvk {

WvkModel::WvkModel(WvkDevice& device, std::string modelFilename) : device{device} {
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

    std::unordered_map<Vertex, uint32_t> indexMap{};

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.position = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.tex_coord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
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

WvkModel::WvkModel(WvkDevice& device, std::vector<Vertex> vertices, std::vector<uint32_t> indices)
                   : device{device}, vertices{vertices}, indices{indices} {
    initialize();
}

void WvkModel::loadModel(std::vector<Vertex> vertices, std::vector<uint32_t> indices) {
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
    VkDevice dev = device.getDevice();

    vertexBuffer.cleanup(dev);
    vertexStagingBuffer.cleanup(dev);

    indexBuffer.cleanup(dev);
    indexStagingBuffer.cleanup(dev);
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



VkVertexInputBindingDescription Vertex::getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};

    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 3> Vertex::getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, position);

    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, tex_coord);

    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].format = VK_FORMAT_R8_UINT;
    attributeDescriptions[2].offset = offsetof(Vertex, texture_index);

    return attributeDescriptions;
}

}
