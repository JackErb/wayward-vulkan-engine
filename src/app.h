#pragma once

#include "wvk_window.h"
#include "wvk_device.h"
#include "wvk_pipeline.h"
#include "wvk_model.h"
#include "wvk_skeleton.h"
#include "wvk_sampler.h"
#include "game/game_structs.h"
#include "glm.h"

#include <vector>
#include <unordered_map>

namespace wvk {

enum KeyState {
    KEY_PRESSED,
    KEY_HELD,
    KEY_RELEASED,
};

enum CursorState {
    CURSOR_ENABLED,
    CURSOR_DISABLED
};

struct ObjectPushConstant {
    uint32_t objectId;
};

struct ObjectData {
    static const int MAX_JOINTS = 16;

    glm::mat4 transform;
    glm::mat4 joints[MAX_JOINTS];
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
    bool isKeyHeld(int);
    bool isKeyReleased(int);

    glm::vec2 getCursorPos();
    bool cursorEnabled() { return window.cursorEnabled(); }
    void enableCursor(bool enable) { window.enableCursor(enable); }

    void setCamera(Camera *camera) { this->camera = camera; }
    void setLight(int light, TransformMatrices *transform);
    void addModel(WvkModel *model) { models.push_back(model); }
    void addSkeleton(WvkSkeleton *skeleton) { skeletons.push_back(skeleton); }

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

    void writeToBuffer(VkDeviceMemory memory, uint32_t size, const void *data);

    void updateKeys();

    std::vector<WvkModel*> models;
    std::vector<WvkSkeleton*> skeletons;

    uint64_t frame = 0;

    WvkWindow window{WIDTH, HEIGHT, "Hello Vulkan!"};
    WvkDevice device{window};
    WvkSwapchain swapChain{device, window.getExtent()};

    Camera *camera = nullptr;

    std::unique_ptr<WvkPipeline> shadowPipeline;
    std::unique_ptr<WvkPipeline> riggedPipeline;
    std::unique_ptr<WvkPipeline> pipeline;

    /* TODO: Read this MAX_OBJECTS using spirv-reflect from shader */
    constexpr static int MAX_OBJECTS = 8;
    ObjectData objectData[MAX_OBJECTS];

    std::vector<VkCommandBuffer> commandBuffers;

    /* Pipeline descriptor set resources */
    Sampler textureSampler{device, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER};
    Sampler depthSampler{device, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER};

    const std::vector<std::string> images = {"hazel.png", "viking_room.png"};
    std::vector<Image> textureImages;

    std::vector<Buffer> cameraTransformBuffers;
    std::vector<Buffer> lightTransformBuffers;
    std::vector<Buffer> objectDataBuffers;

    std::unordered_map<uint16_t, KeyState> keyStates;
};

};
