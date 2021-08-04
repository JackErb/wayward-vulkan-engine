#pragma once

#include "../glm.h"
#include "../wvk_buffer.h"

/*
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
*/

#include <string>
#include <vector>
#include <array>

namespace tinygltf {
    class Model;
    class Node;
    struct Accessor;
};

namespace wvk {

#define MAX_JOINTS 4

struct SkeletonVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;

    unsigned char joints[MAX_JOINTS];
    float jointWeights[MAX_JOINTS];
};

struct SkeletonJoint {
    // translation, rotation, orientation
};

struct SkeletonData {
    std::vector<SkeletonVertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<SkeletonJoint> joints;
};

class Skeleton {
public:
    Skeleton(std::string filename);
    ~Skeleton();

    const std::vector<SkeletonVertex> &getVertices() { return skeletonData.vertices; }
    const std::vector<uint32_t> &getIndices() { return skeletonData.indices; }

private:
    void createSkeleton(const tinygltf::Model &model);
    const tinygltf::Node &getNode(const tinygltf::Model &model);

    SkeletonData skeletonData{};

    // Animation data, other resources
};

uint16_t readUint16(const tinygltf::Model &model, int attributeIndex, int index);
glm::vec2 readVec2f(const tinygltf::Model &model, int attributeIndex, int index);
glm::vec3 readVec3f(const tinygltf::Model &model, int attributeIndex, int index);
glm::vec4 readVec4f(const tinygltf::Model &model, int attributeIndex, int index);
std::array<unsigned char, 4> readVec4b(const tinygltf::Model &model, int attributeIndex, int index);

}
