#pragma once

#include "../glm.h"
#include "../wvk_buffer.h"

#include "../wvk_vertex_attributes.h"

#include <string>
#include <vector>
#include <array>

namespace tinygltf {
    class Model;
    class Node;
    struct Accessor;
    struct Mesh;
    struct Skin;
};

namespace wvk {

struct SkeletonJoint {
    // translation, rotation, orientation
    glm::mat4 model;
};

struct SkeletonData {
    std::vector<RiggedMeshVertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<SkeletonJoint> joints;
};

class Skeleton {
public:
    Skeleton(std::string filename);
    ~Skeleton();

    const std::vector<RiggedMeshVertex> &getVertices() { return skeletonData.vertices; }
    const std::vector<uint32_t> &getIndices() { return skeletonData.indices; }

private:
    void createSkeleton(const tinygltf::Model &model);
    std::vector<const tinygltf::Node *> getRiggedMeshes(const tinygltf::Model &model);

    void readMeshData(const tinygltf::Model &model, const tinygltf::Mesh &mesh);
    void readJointData(const tinygltf::Model &model, const tinygltf::Skin &skin);

    SkeletonData skeletonData{};

    // Animation data, other resources
};

uint16_t readUint16(const tinygltf::Model &model, int attributeIndex, int index);
glm::vec2 readVec2f(const tinygltf::Model &model, int attributeIndex, int index);
glm::vec3 readVec3f(const tinygltf::Model &model, int attributeIndex, int index);
glm::vec4 readVec4f(const tinygltf::Model &model, int attributeIndex, int index);
std::array<unsigned char, 4> readVec4b(const tinygltf::Model &model, int attributeIndex, int index);

}
