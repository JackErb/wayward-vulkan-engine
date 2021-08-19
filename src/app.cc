#include "app.h"

#include "wvk_helper.h"
#include "glm.h"

#include "game/controller.h"

#include <thread>
#include <chrono>

namespace wvk {

WvkApplication::WvkApplication() {
    createPipelineResources();
    logger::debug("Created pipeline resources");

    createPipelines();
    logger::debug("Created application pipelines");

    createCommandBuffers();
    logger::debug("Created command buffers");
}

WvkApplication::~WvkApplication() {
    vkQueueWaitIdle(device.getGraphicsQueue());
    vkQueueWaitIdle(device.getPresentQueue());
    freeCommandBuffers();

    for (auto &image : textureImages) {
        image.cleanup();
    }

    for (auto &buffer : cameraTransformBuffers) {
        buffer.cleanup();
    }

    for (auto &buffer : lightTransformBuffers) {
        buffer.cleanup();
    }

    textureSampler.cleanup();
    depthSampler.cleanup();

    for (WvkModel *model : models) {
        delete model;
    }

    for (WvkSkeleton *skeleton : skeletons) {
        delete skeleton;
    }

    logger::debug("Application shutting down");
}

void checkVkResult(VkResult err) {
    if (err != VK_SUCCESS) {
        logger::fatal_error("Imgui_ImplVulkan_Init call failed with error code: " + std::to_string(err));
    }
}

std::chrono::time_point<std::chrono::high_resolution_clock> getTime() {
    return std::chrono::high_resolution_clock::now();
}

void WvkApplication::run() {
    using namespace std::chrono_literals;
    using namespace std::chrono;

    const int FRAME_INTERVAL = 240;
    long timeCount = 0;

    bool forceQuit = false;

    wayward::DebugController controller{this};

    while (!forceQuit && !glfwWindowShouldClose(window.getGlfwWindow())) {
        auto start = getTime();

        glfwPollEvents();

        int imageIndex = swapChain.acquireNextImage();

        recordCommandBuffer(imageIndex);

        swapChain.submitCommands(commandBuffers[imageIndex], imageIndex);

        auto end = getTime();
        timeCount += duration_cast<microseconds>(end - start).count();

        updateKeys();
        controller.update();

        if (++frame % FRAME_INTERVAL == 0) {
            int avg = timeCount / FRAME_INTERVAL;
            logger::debug("average frame time: " + std::to_string(avg) + " microseconds");
            timeCount = 0;
        }

        if (isKeyPressed(GLFW_KEY_ESCAPE)) {
            forceQuit = true;
        }

        std::this_thread::sleep_for(16.6ms - duration_cast<milliseconds>(end-start));
    }
}

void WvkApplication::createPipelineResources() {
    // Allocate texture images
    for (size_t i = 0; i < images.size(); i++) {
        textureImages.push_back(Image{device, images[i]});
    }

    // Allocate uniform buffers (one per swapchain image)
    VkDeviceSize bufferSize = sizeof(TransformMatrices);
    for (size_t i = 0; i < swapChain.getImageCount(); i++) {
        // TODO: Look into using non-coherent memory and explicitly flush the the device memory
        // to the GPU whenever updated. This can be more efficient, but not all systems will
        // have this memory type available.

        cameraTransformBuffers.emplace_back();
        device.createBuffer(bufferSize,
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            cameraTransformBuffers[i]);

        lightTransformBuffers.emplace_back();
        device.createBuffer(bufferSize,
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            lightTransformBuffers[i]);
    }
}

void WvkApplication::createPipelines() {
    /* Shadow mapping pipeline */

    DescriptorSetInfo shadowDescriptor{};
    auto &shadowLayout = shadowDescriptor.layoutBindings;

    shadowLayout.resize(1);

    shadowLayout[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    shadowLayout[0].count = 1;
    shadowLayout[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    shadowLayout[0].unique = true;
    for (size_t i = 0; i < swapChain.getImageCount(); i++) {
        shadowLayout[0].data[i][0].buffer = lightTransformBuffers[i].buffer;
        shadowLayout[0].data[i][0].size = lightTransformBuffers[i].size;
    }

    shadowPipeline = std::make_unique<WvkPipeline>(device, swapChain, swapChain.getShadowRenderPass(),
                                                   "shadow.vert.spv", "",
                                                   shadowDescriptor,
                                                   WvkPipeline::defaultPipelineConfigInfo(VK_SAMPLE_COUNT_1_BIT));


    /* Main render pass pipeline */

    DescriptorSetInfo mainDescriptor{};
    auto &mainLayout = mainDescriptor.layoutBindings;

    mainLayout.resize(5);

    /* Camera space projection */
    mainLayout[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    mainLayout[0].count = 1;
    mainLayout[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    mainLayout[0].unique = true;
    for (size_t i = 0; i < swapChain.getImageCount(); i++) {
        mainLayout[0].data[i][0].buffer = cameraTransformBuffers[i].buffer;
        mainLayout[0].data[i][0].size = cameraTransformBuffers[i].size;
    }

    /* Array of textures */
    mainLayout[1].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    mainLayout[1].count = textureImages.size();
    mainLayout[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    mainLayout[1].unique = false;
    for (size_t i = 0; i < textureImages.size(); i++) {
        mainLayout[1].data[0][i].imageView = textureImages[i].imageView;
        mainLayout[1].data[0][i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    /* Texture sampler */
    mainLayout[2].type = VK_DESCRIPTOR_TYPE_SAMPLER;
    mainLayout[2].count = 1;
    mainLayout[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    mainLayout[2].unique = false;
    mainLayout[2].data[0][0].sampler = textureSampler.sampler;

    /* Light depth image */
    mainLayout[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    mainLayout[3].count = 1;
    mainLayout[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    mainLayout[3].unique = false;
    mainLayout[3].data[0][0].imageView = swapChain.getShadowDepthImageView();
    mainLayout[3].data[0][0].sampler = depthSampler.sampler;
    mainLayout[3].data[0][0].imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    /* Light space projection */
    mainLayout[4].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    mainLayout[4].count = 1;
    mainLayout[4].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    mainLayout[4].unique = true;
    for (size_t i = 0; i < swapChain.getImageCount(); i++) {
        mainLayout[4].data[i][0].buffer = lightTransformBuffers[i].buffer;
        mainLayout[4].data[i][0].size = lightTransformBuffers[i].size;
    }

    pipeline = std::make_unique<WvkPipeline>(device, swapChain, swapChain.getRenderPass(),
                                             "triangle.vert.spv", "triangle.frag.spv",
                                             mainDescriptor,
                                             WvkPipeline::defaultPipelineConfigInfo(VK_SAMPLE_COUNT_4_BIT));
}

void WvkApplication::createCommandBuffers() {
    uint32_t imageCount = swapChain.getImageCount();
    commandBuffers.resize(imageCount);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = device.getCommandPool();
    allocInfo.commandBufferCount = static_cast<uint32_t>(imageCount);

    VkResult result = vkAllocateCommandBuffers(device.getDevice(), &allocInfo, commandBuffers.data());
    checkVulkanError(result, "failed to create command buffers");
}

void WvkApplication::freeCommandBuffers() {
    vkFreeCommandBuffers(device.getDevice(), device.getCommandPool(),
                         static_cast<uint32_t>(commandBuffers.size()),
                         commandBuffers.data());
    commandBuffers.clear();
}

void WvkApplication::writeTransform(TransformMatrices *matrices, VkDeviceMemory memory) {
    void *pData;
    vkMapMemory(device.getDevice(), memory, 0, sizeof(TransformMatrices), 0, &pData);
    memcpy(pData, matrices, sizeof(TransformMatrices));
    vkUnmapMemory(device.getDevice(), memory);
}

void WvkApplication::recordShadowRenderPass(int imageIndex) {
    VkCommandBuffer commandBuffer = commandBuffers[imageIndex];

    // Begin the main render pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = swapChain.getShadowRenderPass();
    renderPassInfo.framebuffer = swapChain.getShadowFramebuffer();
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChain.getExtent();

    VkClearValue clearValue{};
    clearValue.depthStencil = {1.f, 0};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearValue;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkExtent2D extent = swapChain.getExtent();

    // Set dynamic state for pipeline
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{{0, 0}, swapChain.getExtent()};
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    shadowPipeline->bind(commandBuffer, imageIndex);

    for (WvkModel *model : models) {
        model->bind(commandBuffer);
        model->draw(commandBuffer);
    }

    for (WvkSkeleton *skeleton : skeletons) {
        skeleton->bind(commandBuffer);
        skeleton->draw(commandBuffer);
    }

    vkCmdEndRenderPass(commandBuffer);
}

void WvkApplication::recordMainRenderPass(int imageIndex) {
    VkCommandBuffer commandBuffer = commandBuffers[imageIndex];

    // Begin the main render pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = swapChain.getRenderPass();
    renderPassInfo.framebuffer = swapChain.getFramebuffer(imageIndex);
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChain.getExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {0.01f, 0.01f, 0.01f, 1.0f};
    clearValues[1].depthStencil = {1.f, 0};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkExtent2D extent = swapChain.getExtent();

    // Set dynamic state for pipeline
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{{0, 0}, swapChain.getExtent()};
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    if (camera != nullptr) {
        float aspectRatio = (float) extent.width / (float) extent.height;
        TransformMatrices matrices = camera->transform.perspectiveProjection(aspectRatio);

        writeTransform(&matrices, cameraTransformBuffers[imageIndex].memory);
    }

    pipeline->bind(commandBuffer, imageIndex);

    for (WvkModel *model : models) {
        model->bind(commandBuffer);
        model->draw(commandBuffer);
    }

    for (WvkSkeleton *skeleton : skeletons) {
        skeleton->bind(commandBuffer);
        skeleton->draw(commandBuffer);
    }

    vkCmdEndRenderPass(commandBuffer);
}

void WvkApplication::recordCommandBuffer(int imageIndex) {
    VkCommandBuffer commandBuffer = commandBuffers[imageIndex];

    // Begin recording to the command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    checkVulkanError(vkBeginCommandBuffer(commandBuffer, &beginInfo), "failed to begin command buffer");

    recordShadowRenderPass(imageIndex);
    recordMainRenderPass(imageIndex);

    checkVulkanError(vkEndCommandBuffer(commandBuffer), "failed to record command buffer");
}

void WvkApplication::updateKeys() {
    for (std::pair<uint16_t, KeyState> pair : keyStates) {
        uint16_t key = pair.first;
        KeyState state = pair.second;

        int glfwState = glfwGetKey(window.getGlfwWindow(), key);

        switch (state) {
        case KEY_PRESSED:
            keyStates[key] = KEY_HELD;
        case KEY_HELD:
            if (glfwState == GLFW_RELEASE) {
                keyStates[key] = KEY_RELEASED;
            }
            break;
        case KEY_RELEASED:
            if (glfwState == GLFW_PRESS) {
                keyStates[key] = KEY_PRESSED;
            }
            break;
        }
    }
}

bool WvkApplication::isKeyPressed(int key) {
    if (keyStates.find(key) == keyStates.end()) {
        keyStates[key] = KEY_RELEASED;
    }
    return keyStates[key] == KEY_PRESSED;
}

bool WvkApplication::isKeyHeld(int key) {
    if (keyStates.find(key) == keyStates.end()) {
        keyStates[key] = KEY_RELEASED;
    }
    return keyStates[key] == KEY_HELD || keyStates[key] == KEY_PRESSED;
}

bool WvkApplication::isKeyReleased(int key) {
    if (keyStates.find(key) == keyStates.end()) {
        keyStates[key] = KEY_RELEASED;
    }
    return keyStates[key] == KEY_RELEASED;
}

glm::vec2 WvkApplication::getCursorPos() {
    double cx, cy;
    glfwGetCursorPos(window.getGlfwWindow(), &cx, &cy);

    return glm::vec2(cx, cy);
}

void WvkApplication::setLight(int light, TransformMatrices *transform) {
    // TODO: param light index is currently unused

    for (size_t i = 0; i < lightTransformBuffers.size(); i++) {
        writeTransform(transform, lightTransformBuffers[i].memory);
    }
}

}
