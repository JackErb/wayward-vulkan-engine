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

std::vector<const tinygltf::Node *> Skeleton::getRiggedMeshes(const tinygltf::Model &model) {
    std::vector<const tinygltf::Node *> nodes;

    for (size_t i = 0; i < model.nodes.size(); i++) {
        if (model.nodes[i].skin != -1 && model.nodes[i].mesh != -1) {
            logger::debug("Found main node: " + model.nodes[i].name);
            nodes.push_back(&model.nodes[i]);
        }
    }

    return nodes;
}

void Skeleton::readMeshData(const tinygltf::Model &model, const tinygltf::Mesh &mesh) {
    tinygltf::Primitive primitive = mesh.primitives[0];
    if (mesh.primitives.size() > 1) {
        logger::fatal_error("More than one primitive detected for GLTF file, only one supported.");
    }

    // Read the vertices
    int vertexCount = model.accessors[primitive.attributes["POSITION"]].count;

    size_t verticesOffset = skeletonData.vertices.size();
    skeletonData.vertices.resize(skeletonData.vertices.size() + vertexCount);

    for (size_t vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++) {
        RiggedMeshVertex vertex{};

        vertex.position = readVec3f(model, primitive.attributes["POSITION"], vertexIndex);
        vertex.normal = readVec3f(model, primitive.attributes["NORMAL"], vertexIndex);
        vertex.tex_coord = readVec2f(model, primitive.attributes["TEXCOORD_0"], vertexIndex);
        vertex.texture_index = 0; // TODO: Read this value properly

        auto joints = readVec4b(model, primitive.attributes["JOINTS_0"], vertexIndex);
        vertex.joint1 = joints[0];
        vertex.joint2 = joints[1];

        glm::vec4 weights = readVec4f(model, primitive.attributes["WEIGHTS_0"], vertexIndex);
        vertex.weight1 = weights[0];
        vertex.weight2 = weights[1];

        skeletonData.vertices[verticesOffset + vertexIndex] = vertex;
        logger::debug(vertexIndex);
    }
    logger::debug("Finished reading vertex data. count: " + std::to_string(vertexCount));

    // Read the indices
    int indices = primitive.indices; // The accessor index containing indice data
    if ( !(indices >= 0 && primitive.mode == TINYGLTF_MODE_TRIANGLES) ) {
        logger::fatal_error("GLTF mesh encoding must be triangles w/ indices");
    }

    int indexCount = model.accessors[indices].count;

    size_t indicesOffset = skeletonData.indices.size();
    skeletonData.indices.resize(skeletonData.indices.size() + indexCount);

    for (size_t indiceIndex = 0; indiceIndex < indexCount; indiceIndex++) {
        int indice = readUint16(model, indices, indiceIndex);

        skeletonData.indices[indicesOffset + indiceIndex] = verticesOffset + indice;
    }
}

void Skeleton::readJointData(const tinygltf::Model &model, const tinygltf::Skin &skin) {
    for (int joint : skin.joints) {
        logger::debug("Joint: " + model.nodes[joint].name);
    }
}

void Skeleton::createSkeleton(const tinygltf::Model &model) {
    logger::debug("Creating skeleton");

    for (const tinygltf::Node *node : getRiggedMeshes(model)) {
        const tinygltf::Skin &skin = model.skins[node->skin];
        const tinygltf::Mesh &mesh = model.meshes[node->mesh];
        
        logger::debug("Node: " + node->name);
        logger::debug("Skin: " + skin.name);
        logger::debug("Mesh: " + mesh.name);

        readMeshData(model, mesh);
        readJointData(model, skin);
        //readAnimationData();
    }
    
    logger::debug("Finished reading skeletal data");
}

Skeleton::~Skeleton() {

}

}
