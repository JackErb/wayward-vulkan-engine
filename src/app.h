#pragma once

#include "wvk_window.h"
#include "wvk_device.h"
#include "wvk_pipeline.h"
#include "wvk_model.h"
#include "wvk_skeleton.h"
#include "wvk_sampler.h"
#include "game/game_structs.h"

#include <vector>

namespace wvk {

class WvkApplication {
  public:
    static constexpr int WIDTH = 800;
    static constexpr int HEIGHT = 600;

    WvkApplication();
    ~WvkApplication();

    WvkApplication(const WvkApplication &) = delete;
    WvkApplication &operator=(const WvkApplication &) = delete;

    void run();

    bool isKeyPressed(int);
    glm::vec2 getCursorPos();

    void setCamera(Camera *camera) { this->camera = camera; }
    void setLight(int light, TransformMatrices *transform);
    void addModel(WvkModel *model) { models.push_back(model); }

    uint64_t getFrame() { return frame; }

    WvkDevice &getDevice() { return device; }

  private:
    void createPipelineResources();
    void createPipelines();

    void createCommandBuffers();
    void freeCommandBuffers();
    void recordCommandBuffer(int imageIndex);

    void recordShadowRenderPass(int imageIndex);
    void recordMainRenderPass(int imageIndex);

    void writeTransform(TransformMatrices *transform, VkDeviceMemory memory);

    uint64_t frame = 0;

    WvkWindow window{WIDTH, HEIGHT, "Hello Vulkan!"};
    WvkDevice device{window};
    WvkSwapchain swapChain{device, window.getExtent()};

    Camera *camera = nullptr;

    //WvkPipeline pipeline{device, swapChain, swapChain.getRenderPass(), "triangle.vert.spv", "triangle.frag.spv", WvkPipeline::defaultPipelineConfigInfo()};
    //WvkPipeline shadowPipeline{device, swapChain, swapChain.getShadowRenderPass(), "triangle.vert.spv", "", WvkPipeline::defaultPipelineConfigInfo()};
    std::unique_ptr<WvkPipeline> pipeline;
    std::unique_ptr<WvkPipeline> shadowPipeline;

    std::vector<WvkModel*> models;
    std::vector<WvkSkeleton*> skeletons;

    std::vector<VkCommandBuffer> commandBuffers;

    /* Pipeline descriptor set resources */
    Sampler textureSampler{device};
    Sampler depthSampler{device};

    const std::vector<std::string> images = {"hazel.png", "viking_room.png"};
    std::vector<Image> textureImages;

    std::vector<Buffer> cameraTransformBuffers;
    std::vector<Buffer> lightTransformBuffers;
};

};
