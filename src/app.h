#pragma once

#include "wvk_window.h"
#include "wvk_device.h"
#include "wvk_pipeline.h"
#include "wvk_model.h"
#include "wvk_skeleton.h"
#include "wvk_sampler.h"

#include <vector>

namespace wvk {

struct CameraTransform {
    glm::vec3 position;

    glm::vec3 lookingAt;

};

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

  private:
    void createPipelineResources();
    void createPipelines();

    void createCommandBuffers();
    void freeCommandBuffers();
    void recordCommandBuffer(int imageIndex);

    void recordShadowRenderPass(int imageIndex);
    void recordMainRenderPass(int imageIndex);

    void updateCamera(int imageIndex, VkExtent2D extent, float offset);

    void loadModels();
    void imguiInit();

    uint64_t frame = 0;

    WvkWindow window{WIDTH, HEIGHT, "Hello Vulkan!"};
    WvkDevice device{window};
    WvkSwapchain swapChain{device, window.getExtent()};

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

    struct {
        glm::vec3 position; // position in worldspace
        glm::vec3 rotation; // components:  pitch, yaw, roll
        glm::vec3 direction;

        glm::vec2 mousePosition;
    } camera;
    std::vector<Buffer> cameraTransformBuffers;
};

};
