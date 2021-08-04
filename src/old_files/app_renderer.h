#pragma once

#include <logger.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

class renderer {
public:
    renderer();
    ~renderer();

    renderer(const auto &renderer) = delete;
    renderer &operator=(const auto &renderer) = delete;

    std::vector<VkCommandBuffer> record_command_buffers();

    void add_model(model_buffer model) { models.push_back(model); }

private:
    std::vector<model_buffer> models;

    VkExtent2D extent;
    VkPipelineLayout pipeline_layout;
    VkRenderPass render_pass;
};
