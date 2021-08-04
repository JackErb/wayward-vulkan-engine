#pragma once

#include "wvk_window.h"
#include "wvk_device.h"
#include "wvk_pipeline.h"
#include "wvk_model.h"
#include "wvk_skeleton.h"

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
  
  private:
    void createCommandBuffers();
    void freeCommandBuffers();
    void recordCommandBuffer(int imageIndex);

    void loadModels();
    void imguiInit();

    WvkWindow window{WIDTH, HEIGHT, "Hello Vulkan!"};
    WvkDevice device{window};
    WvkSwapchain swapChain{device, window.getExtent()};
    WvkPipeline pipeline{device, swapChain, swapChain.getRenderPass(), "triangle.vert.spv", "triangle.frag.spv", WvkPipeline::defaultPipelineConfigInfo(swapChain.getExtent())};

    std::vector<WvkModel*> models;
    std::vector<WvkSkeleton*> skeletons;

    std::vector<VkCommandBuffer> commandBuffers;
};

};
