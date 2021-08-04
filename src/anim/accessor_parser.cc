#include "skeleton.h"

#include <logger.h>
#include <tiny_gltf.h>

namespace wvk {

unsigned char *getAttributePointer(const tinygltf::Model &model, const tinygltf::Accessor &accessor,
                                   int index, int stride) {
    tinygltf::BufferView bufferView = model.bufferViews[accessor.bufferView];
    tinygltf::Buffer buffer = model.buffers[bufferView.buffer];

    int offset = bufferView.byteOffset + accessor.byteOffset;
    unsigned char *accessorData = &buffer.data[offset];

    int accessorWidth = tinygltf::GetComponentSizeInBytes(accessor.componentType);
    int dimensionality;

    switch (accessor.type) {
    case TINYGLTF_TYPE_VEC2:
    case TINYGLTF_TYPE_VEC3:
    case TINYGLTF_TYPE_VEC4:
        dimensionality = accessor.type; // 2, 3, or 4
        break;
    case TINYGLTF_TYPE_SCALAR:
        dimensionality = 1; // A scalar represents just one value
        break;
    default:
        logger::fatal_error("Unsupported accessor type when loading GLTF file");
    }


    return accessorData + index * accessorWidth * dimensionality;
}

uint16_t readUint16(const tinygltf::Model &model, int attributeIndex, int accessorIndex) {
    tinygltf::Accessor accessor = model.accessors[attributeIndex];
    if (accessor.type != TINYGLTF_TYPE_SCALAR || accessor.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        logger::debug("type: " + std::to_string(accessor.type) + ", component type: " + std::to_string(accessor.componentType));
        logger::fatal_error("readVec2Attribute :: accessor does not have correct dimensionality and/or type");
    }

    int stride = sizeof(uint16_t);
    unsigned char *accessorData = getAttributePointer(model, accessor, accessorIndex, stride);

    uint16_t *value = reinterpret_cast<uint16_t*>(accessorData);
    uint16_t result = static_cast<uint16_t>(*value);

    return result;
}

glm::vec2 readVec2f(const tinygltf::Model &model, int attributeIndex, int accessorIndex) {
    tinygltf::Accessor accessor = model.accessors[attributeIndex];
    if (accessor.type != 2 || accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
        logger::fatal_error("readVec2Attribute :: accessor does not have correct dimensionality and/or type");
    }

    glm::vec2 result;

    int dimensionality = accessor.type;
    int stride = sizeof(float);
    unsigned char *accessorData = getAttributePointer(model, accessor, accessorIndex, stride);

    for (size_t index = 0; index < dimensionality; index++) {
        float *value = reinterpret_cast<float*>(accessorData + index * stride);
        result[index] = *value;
    }

    return result;
}

glm::vec3 readVec3f(const tinygltf::Model &model, int attributeIndex, int accessorIndex) {
    tinygltf::Accessor accessor = model.accessors[attributeIndex];
    if (accessor.type != 3 || accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
        logger::fatal_error("readVec2Attribute :: accessor does not have correct dimensionality and/or type");
    }

    glm::vec3 result;

    int dimensionality = accessor.type;
    int stride = sizeof(float);
    unsigned char *accessorData = getAttributePointer(model, accessor, accessorIndex, stride);

    for (size_t index = 0; index < dimensionality; index++) {
        float *value = reinterpret_cast<float*>(accessorData + index * stride);
        result[index] = *value;
    }

    return result;
}

glm::vec4 readVec4f(const tinygltf::Model &model, int attributeIndex, int accessorIndex) {
    tinygltf::Accessor accessor = model.accessors[attributeIndex];
    if (accessor.type != 4 || accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
        logger::fatal_error("readVec2Attribute :: accessor does not have correct dimensionality and/or type");
    }

    glm::vec4 result;

    int dimensionality = accessor.type;
    int stride = sizeof(float);
    unsigned char *accessorData = getAttributePointer(model, accessor, accessorIndex, stride);

    for (size_t index = 0; index < dimensionality; index++) {
        float *value = reinterpret_cast<float*>(accessorData + index * stride);
        result[index] = *value;
    }

    return result;
}

std::array<unsigned char, 4> readVec4b(const tinygltf::Model &model, int attributeIndex, int accessorIndex) {
    tinygltf::Accessor accessor = model.accessors[attributeIndex];
    if (accessor.type != 4 || accessor.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
        logger::fatal_error("readVec2Attribute :: accessor does not have correct dimensionality and/or type");
    }

    std::array<unsigned char, 4> result;

    int dimensionality = accessor.type;
    int stride = sizeof(unsigned char);
    unsigned char *accessorData = getAttributePointer(model, accessor, accessorIndex, stride);

    for (size_t index = 0; index < dimensionality; index++) {
        unsigned char *value = reinterpret_cast<unsigned char*>(accessorData + index * stride);
        result[index] = *value;
    }

    return result;
}

}
