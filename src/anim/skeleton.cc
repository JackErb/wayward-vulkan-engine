#include "skeleton.h"

#include <tiny_gltf.h>

#include <logger.h>

#include "../resource_path.h"

namespace wvk {

Skeleton::Skeleton(std::string filename) {
    std::string path = resourcePath() + filename;

    tinygltf::TinyGLTF loader{};
    tinygltf::Model model{};
    std::string error;
    std::string warn;

    if (!loader.LoadBinaryFromFile(&model, &error, &warn, path.c_str())) {
        logger::error(warn);
        logger::error(error);
        logger::fatal_error("Failed to load .glb file");
    }

    if (!warn.empty()) {
        logger::debug(warn);
    }

    createSkeleton(model);
}

const tinygltf::Node &Skeleton::getNode(const tinygltf::Model &model) {
    const tinygltf::Node *node;
    for (size_t i = 0; i < model.nodes.size(); i++) {
        if (model.nodes[i].name.compare("Cube") == 0) {
            node = &model.nodes[i];
        }
    }

    if (node->skin == -1) {
        logger::fatal_error("Node does not contain skin (skeletal) data");
    }

    if (node->mesh == -1) {
        logger::fatal_error("Node does not contain mesh data");
    }

    return *node;
}

void Skeleton::createSkeleton(const tinygltf::Model &model) {
    const tinygltf::Node &node = getNode(model);

    tinygltf::Skin skin = model.skins[node.skin];
    tinygltf::Mesh mesh = model.meshes[node.mesh];

    logger::debug("Node: " + node.name);
    logger::debug("Skin: " + skin.name);
    logger::debug("Mesh: " + mesh.name);

    for (int joint : skin.joints) {
        logger::debug("Joint: " + model.nodes[joint].name);
    }
    logger::debug("");

    tinygltf::Primitive primitive = mesh.primitives[0];
    if (mesh.primitives.size() > 1) {
        logger::fatal_error("More than one primitive detected for GLTF file, only one supported.");
    }

    // Read the vertex data
    int vertexCount = model.accessors[primitive.attributes["POSITION"]].count;
    for (size_t vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++) {
        SkeletonVertex vertex{};
        vertex.position = readVec3f(model, primitive.attributes["POSITION"], vertexIndex);
        vertex.normal = readVec3f(model, primitive.attributes["NORMAL"], vertexIndex);
        vertex.texCoord = readVec2f(model, primitive.attributes["TEXCOORD_0"], vertexIndex);

        auto joints = readVec4b(model, primitive.attributes["JOINTS_0"], vertexIndex);
        for (size_t i = 0; i < joints.size(); i++) {
            vertex.joints[i] = joints[i];
        }
        glm::vec4 weights = readVec4f(model, primitive.attributes["WEIGHTS_0"], vertexIndex);
        for (size_t i = 0; i < joints.size(); i++) {
            vertex.jointWeights[i] = weights[i];
        }

        skeletonData.vertices.push_back(vertex);
    }

    // Read the index data
    int indices = primitive.indices; // The accessor index containing indice data
    if ( !(indices >= 0 && primitive.mode == TINYGLTF_MODE_TRIANGLES) ) {
        logger::fatal_error("GLTF mesh encoding must be triangles w/ indices");
    }

    for (size_t indexCount = 0; indexCount < model.accessors[indices].count; indexCount++) {
        int indice = readUint16(model, indices, indexCount);

        skeletonData.indices.push_back(indice);
    }
}

Skeleton::~Skeleton() {

}

}
