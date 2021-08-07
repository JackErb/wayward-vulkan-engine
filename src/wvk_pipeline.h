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

#define MAX_DESCRIPTOR_COUNT 10 // max size of descriptor array
#define MAX_DESCRIPTORS      10 // max number of descriptors for each type (buffer, image, sampler)

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

struct DescriptorLayoutInfo {
    VkDescriptorType type;
    uint32_t count;
    VkShaderStageFlags stageFlags;

    // If true, this descriptor  should be unique for each frame of the swap chain.
    //          The length of the data array must be 1
    // If false, the same data will be used for each frame.
    //           The length of the data array must be the swap chain's image count
    // Cases this should be true:
    // - Data that is mutable (can change from frame to frame)
    // - Framebuffer attachments from previous render passes
    bool unique = false;

    // TODO: This is messy and unsafe. Find more suitable design
    union {
        VkSampler   sampler;
        VkImageView imageView;
        VkBuffer    buffer;
    } data[WvkSwapchain::MAX_FRAMES_IN_FLIGHT][MAX_DESCRIPTOR_COUNT];
};

struct DescriptorSetInfo {
    std::vector<DescriptorLayoutInfo> layoutBindings;
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
                const DescriptorSetInfo &descriptorInfo,
                const PipelineConfigInfo &config);
    ~WvkPipeline();

    WvkPipeline(const WvkPipeline&) = delete;
    WvkPipeline& operator=(const WvkPipeline&) = delete;

    static PipelineConfigInfo defaultPipelineConfigInfo();

    void updateUniformBuffer(int imageIndex, UniformBufferObject ubo);
    void bind(VkCommandBuffer commandBuffer, int imageIndex);

  private:
    void createGraphicsPipeline(std::string vertShader, std::string fragShader, const PipelineConfigInfo &config);

    void createPipelineLayout();
    void createDescriptorPool();
    void createDescriptorSets();
    VkShaderModule createShaderModule(const std::string &filename);

    WvkDevice& device;
    WvkSwapchain& swapChain;
    VkRenderPass renderPass;

    PipelineConfigInfo pipelineConfig;

    DescriptorSetInfo descriptorSetInfo;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    VkPipeline graphicsPipeline;
    VkShaderModule vertShaderModule = VK_NULL_HANDLE;
    VkShaderModule fragShaderModule = VK_NULL_HANDLE;
};

}
