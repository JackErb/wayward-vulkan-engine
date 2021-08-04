#include "wvk_skeleton.h"

namespace wvk {

std::vector<Vertex> skeletonVertexToWvkVertex(const std::vector<SkeletonVertex> vertices) {
    std::vector<Vertex> converted;
    for (auto skeletonVertex : vertices) {
        Vertex vertex{skeletonVertex.position, skeletonVertex.texCoord};
        converted.push_back(vertex);
    }
    return converted;
}

WvkSkeleton::WvkSkeleton(WvkDevice& device, std::string filename) :
    device{device}, skeleton{filename}, model{device} {

    std::vector<Vertex> vertices = skeletonVertexToWvkVertex(skeleton.getVertices());
    model.loadModel(vertices, skeleton.getIndices());

}

void WvkSkeleton::bind(VkCommandBuffer commandBuffer) {
    VkBuffer vertexBuffer = model.getVertexBuffer();
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);

    VkBuffer indexBuffer = model.getIndexBuffer();
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, offset, VK_INDEX_TYPE_UINT32);
}

void WvkSkeleton::draw(VkCommandBuffer commandBuffer) {
    uint32_t numIndices = model.getIndices().size();
    vkCmdDrawIndexed(commandBuffer, numIndices, 1, 0, 0, 0);
}

}
