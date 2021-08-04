#pragma once

#include "wvk_device.h"
#include "wvk_image.h"
#include "wvk_buffer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <vector>
#include <array>

#include <memory>

namespace wvk {

struct PipelineConfigInfo {
  VkPipelineViewportStateCreateInfo viewportInfo;
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
  VkPipelineRasterizationStateCreateInfo rasterizationInfo;
  VkPipelineMultisampleStateCreateInfo multisampleInfo;
  VkPipelineColorBlendAttachmentState colorBlendAttachment;
  VkPipelineColorBlendStateCreateInfo colorBlendInfo;
  VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
  std::vector<VkDynamicState> dynamicStateEnables;
  VkPipelineDynamicStateCreateInfo dynamicStateInfo;
  uint32_t subpass = 0;
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
};

class WvkPipeline {
  public:
    WvkPipeline(WvkDevice& device, WvkSwapchain& swapChain, VkRenderPass renderPass,
                std::string vertShader, std::string fragShader,
                const PipelineConfigInfo &config);
    ~WvkPipeline();

    WvkPipeline(const WvkPipeline&) = delete;
    WvkPipeline& operator=(const WvkPipeline&) = delete;

    static PipelineConfigInfo defaultPipelineConfigInfo(VkExtent2D extent);

    void updateUniformBuffer(int imageIndex);
    void bind(VkCommandBuffer commandBuffer, int imageIndex);

  private:
    void createGraphicsPipeline(std::string vertShader, std::string fragShader, const PipelineConfigInfo &config);

    void createPipelineLayout();
    void createDescriptorPool();
    void createDescriptorResources();
    void createDescriptorSets();
    VkShaderModule createShaderModule(const std::string &filename);

    WvkDevice& device;
    WvkSwapchain& swapChain;
    VkRenderPass renderPass;

    PipelineConfigInfo pipelineConfig;

    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    VkPipeline graphicsPipeline;
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;

    VkSampler textureSampler;
    std::vector<WvkBuffer> uniformBuffers;

    const std::vector<std::string> images = {"viking_room.png", "hazel.png"};
    std::vector<std::unique_ptr<WvkImage>> textureImages;
};

}
