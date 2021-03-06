#pragma once

#include "wvk_device.h"
#include "wvk_image.h"
#include "wvk_buffer.h"
#include "game/game_structs.h"
#include "wvk_swapchain.h"

#include "glm.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

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
    struct {
        VkSampler         sampler = VK_NULL_HANDLE;
        VkImageView     imageView = VK_NULL_HANDLE;
        VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkBuffer     buffer = VK_NULL_HANDLE;
        VkDeviceSize   size = 0;
    } data[WvkSwapchain::MAX_FRAMES_IN_FLIGHT][MAX_DESCRIPTOR_COUNT];
};


struct VertexDescriptionInfo {
    VkVertexInputBindingDescription binding;
    std::vector<VkVertexInputAttributeDescription> attributes;
};

struct DescriptorSetInfo {
    std::vector<DescriptorLayoutInfo> layoutBindings;
};

struct PushConstantInfo {
    std::vector<VkPushConstantRange> pushConstants;
};

class WvkPipeline {
  public:
    WvkPipeline(WvkDevice& device, WvkSwapchain& swapChain, VkRenderPass renderPass,
                std::string vertShader, std::string fragShader,
                const PushConstantInfo &pushInfo,
                const DescriptorSetInfo &descriptorInfo,
                const VertexDescriptionInfo &vertexInfo,
                const PipelineConfigInfo &config);
    ~WvkPipeline();

    WvkPipeline(const WvkPipeline&) = delete;
    WvkPipeline& operator=(const WvkPipeline&) = delete;

    static PipelineConfigInfo defaultPipelineConfigInfo(VkSampleCountFlagBits samples);

    void updateUniformBuffer(int imageIndex, TransformMatrices ubo);
    void bind(VkCommandBuffer commandBuffer, int imageIndex);

    VkPipelineLayout getPipelineLayout() { return pipelineLayout; }

  private:
    void createGraphicsPipeline(std::string vertShader, std::string fragShader, const PipelineConfigInfo &config, const VertexDescriptionInfo &vertexInfo);

    void createPipelineLayout();
    void createDescriptorPool();
    void createDescriptorSets();
    VkShaderModule createShaderModule(const std::string &filename);

    WvkDevice& device;
    WvkSwapchain& swapChain;
    VkRenderPass renderPass;

    PipelineConfigInfo pipelineConfig;

    PushConstantInfo pushConstantInfo;

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
