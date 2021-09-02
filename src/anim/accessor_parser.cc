#include "skeleton.h"

#include <logger.h>
#include <tiny_gltf.h>

namespace wvk {

const unsigned char *getAttributePointer(const tinygltf::Model &model, const tinygltf::Accessor &accessor,
                                   int index, int stride) {
    if (index >= accessor.count) {
        logger::fatal_error("Tried to index gltf attribute out of bounds");
    }
    const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer     &buffer     = model.buffers[bufferView.buffer];

    int offset = bufferView.byteOffset + accessor.byteOffset;
    const unsigned char *accessorData = &buffer.data[offset];

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
        logger::fatal_error("readUint16 :: accessor does not have correct dimensionality and/or type");
    }

    int stride = sizeof(uint16_t);
    const unsigned char *accessorData = getAttributePointer(model, accessor, accessorIndex, stride);

    uint16_t result = *reinterpret_cast<const uint16_t*>(accessorData + 0 * stride);

    return result;
}

glm::vec2 readVec2f(const tinygltf::Model &model, int attributeIndex, int accessorIndex) {
    tinygltf::Accessor accessor = model.accessors[attributeIndex];
    if (accessor.type != 2 || accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
        logger::fatal_error("readVec2f :: accessor does not have correct dimensionality and/or type");
    }

    glm::vec2 result;

    int dimensionality = accessor.type;
    int stride = sizeof(float);
    const unsigned char *accessorData = getAttributePointer(model, accessor, accessorIndex, stride);

    result.x = *reinterpret_cast<const float*>(accessorData + 0 * stride);
    result.y = *reinterpret_cast<const float*>(accessorData + 1 * stride);

    return result;
}

glm::vec3 readVec3f(const tinygltf::Model &model, int attributeIndex, int accessorIndex) {
    tinygltf::Accessor accessor = model.accessors[attributeIndex];
    if (accessor.type != 3 || accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
        logger::fatal_error("readVec3f :: accessor does not have correct dimensionality and/or type");
    }

    glm::vec3 result;

    int dimensionality = accessor.type;
    int stride = sizeof(float);
    const unsigned char *accessorData = getAttributePointer(model, accessor, accessorIndex, stride);
    
    result.x = *reinterpret_cast<const float*>(accessorData + 0 * stride);
    result.y = *reinterpret_cast<const float*>(accessorData + 1 * stride);
    result.z = *reinterpret_cast<const float*>(accessorData + 2 * stride);

    return result;
}

glm::vec4 readVec4f(const tinygltf::Model &model, int attributeIndex, int accessorIndex) {
    tinygltf::Accessor accessor = model.accessors[attributeIndex];
    if (accessor.type != 4 || accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
        logger::fatal_error("readVec4f :: accessor does not have correct dimensionality and/or type");
    }

    glm::vec4 result;

    int dimensionality = accessor.type;
    int stride = sizeof(float);
    const unsigned char *accessorData = getAttributePointer(model, accessor, accessorIndex, stride);
    
    result.x = *reinterpret_cast<const float*>(accessorData + 0 * stride);
    result.y = *reinterpret_cast<const float*>(accessorData + 1 * stride);
    result.z = *reinterpret_cast<const float*>(accessorData + 2 * stride);
    result.w = *reinterpret_cast<const float*>(accessorData + 3 * stride);

    return result;
}

std::array<unsigned char, 4> readVec4b(const tinygltf::Model &model, int attributeIndex, int accessorIndex) {
    tinygltf::Accessor accessor = model.accessors[attributeIndex];
    if (accessor.type != 4 || accessor.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
        logger::fatal_error("readVec2Attribute :: accessor does not have correct dimensionality and/or type");
    }

    std::array<unsigned char, 4> result;

    int dimensionality = accessor.type;
    int stride = sizeof(const unsigned char *);
    const unsigned char *accessorData = getAttributePointer(model, accessor, accessorIndex, stride);

    result[0] = *(accessorData + 0 * stride);
    result[1] = *(accessorData + 1 * stride);
    result[2] = *(accessorData + 2 * stride);
    result[3] = *(accessorData + 3 * stride);

    return result;
}

}
