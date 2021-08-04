#pragma once

#include "attributes.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

struct ModelMesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    std::vector<Vertex> resolve_mesh(glm::vec2 position);
};

struct data_buffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
}

class model_buffer {
public:
    model_buffer();
    ~model_buffer();

    model_buffer(const auto &model_buffer) = delete;
    model_buffer &operator=(const auto &model_buffer) = delete;

    // Fill internal data structures representing mesh with data from the file
    // TODO: Accept type parameter to specify file type.
    void create_mesh(std::string fileName);

    // To be called each frame to record the updated vertex positions
    void update_buffer();
    VkBuffer get_buffer() { return vertex_buffer; }

    void set_position(glm::vec2 pos) { position = pos; }
    VkBuffer get_vertex_buffer() { return vertex_buffer.buffer; }
    VkBuffer get_index_buffer() { return index_buffer.buffer; }
    uint32_t num_vertices() { return mesh.vertices.size(); }

private:
    ModelMesh mesh;
    glm::vec3 position;

    data_buffer vertex_buffer;
    data_buffer index_buffer;
};
