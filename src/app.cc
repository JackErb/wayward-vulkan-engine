#include "app.h"

#include "wvk_helper.h"
#include "glm.h"

#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

#include <thread>
#include <chrono>

#include "glm.h"

namespace wvk {

WvkApplication::WvkApplication() {
    createCommandBuffers();
    logger::debug("Created command buffers");

    loadModels();
    logger::debug("Load models");

    imguiInit();
}

WvkApplication::~WvkApplication() {
    vkQueueWaitIdle(device.getGraphicsQueue());
    vkQueueWaitIdle(device.getPresentQueue());
    freeCommandBuffers();

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

void WvkApplication::imguiInit() {
    /*
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(device.getWindow().getGlfwWindow(), true);

    ImGui_ImplVulkan_InitInfo initInfo{};
    init_info.Instance = device.getInstance();
    init_info.PhysicalDevice = device.getPhysicalDevice();
    init_info.Device = device.getDevice();
    init_info.QueueFamily = device.getQueueIndices().graphicsQueue;
    init_info.Queue = device.getGraphicsQueue();
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = VK_NULL_HANDLE;
    init_info.Allocator = nullptr;
    init_info.MinImageCount = 2;
    init_info.ImageCount = swapChain.getImageCount();
    init_info.CheckVkResultFn = &checkVkResult;
    ImGui_ImplVulkan_Init(&initInfo, wd->RenderPass);
    */
}

std::chrono::time_point<std::chrono::high_resolution_clock> getTime() {
    return std::chrono::high_resolution_clock::now();
}

void WvkApplication::run() {
    using namespace std::chrono_literals;
    using namespace std::chrono;

    const int FRAME_INTERVAL = 240;
    long frame = 0;
    long timeCount = 0;

    while (!glfwWindowShouldClose(window.getGlfwWindow())) {
        auto start = getTime();

        glfwPollEvents();

        int imageIndex = swapChain.acquireNextImage();

        recordCommandBuffer(imageIndex);

        swapChain.submitCommands(commandBuffers[imageIndex], imageIndex);

        auto end = getTime();
        timeCount += duration_cast<microseconds>(end - start).count();
        if (frame % FRAME_INTERVAL == 0) {
            int avg = timeCount / FRAME_INTERVAL;
            logger::debug("average frame time: " + std::to_string(avg) + " microseconds");
            timeCount = 0;
        }

        frame++;

        std::this_thread::sleep_for(16.6ms - duration_cast<milliseconds>(end-start));
    }
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

void mainRenderPass() {

}

void WvkApplication::recordCommandBuffer(int imageIndex) {
    VkCommandBuffer commandBuffer = commandBuffers[imageIndex];

    // Begin recording to the command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    checkVulkanError(result, "failed to begin command buffer");

    // Begin the light render pass

    // Begin the main render pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = swapChain.getRenderPass();
    renderPassInfo.framebuffer = swapChain.getFramebuffer(imageIndex);
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChain.getExtent();

    std::array<VkClearValue, 3> clearValues{};
    clearValues[0].color = {0.01f, 0.01f, 0.01f, 1.0f};
    clearValues[1].depthStencil = {1.f, 0};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Set dynamic state for pipeline
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChain.getExtent().width);
    viewport.height = static_cast<float>(swapChain.getExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{{0, 0}, swapChain.getExtent()};
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    pipeline.bind(commandBuffer, imageIndex);

    for (WvkModel *model : models) {
        model->bind(commandBuffer);
        model->draw(commandBuffer);
    }

    for (WvkSkeleton *skeleton : skeletons) {
        skeleton->bind(commandBuffer);
        skeleton->draw(commandBuffer);
    }

    vkCmdEndRenderPass(commandBuffer);
    checkVulkanError(vkEndCommandBuffer(commandBuffer), "failed to record command buffer");
}

void WvkApplication::loadModels() {
    /*std::vector<Vertex> vertices = {
        {{-0.5f, -0.5, 1.f}, {0.f, 0.f}},
        {{-0.5f, 0.5f, 1.f}, {0.f, 1.f}},
        {{0.5f, 0.5f, 1.f}, {1.f, 1.f}},
        {{0.5f, -0.5f, 1.f}, {1.f, 0.f}}
    };

    std::vector<Vertex> allVertices{};
    std::vector<uint32_t> allIndices{};
    for (int i = 0; i < 10; i++) {
        vertices[0].position.z = 1.f - 0.1f * i;
        vertices[1].position.z = 1.f - 0.1f * i;
        vertices[2].position.z = 1.f - 0.1f * i;
        vertices[3].position.z = 1.f - 0.1f * i;

        allVertices.push_back(vertices[0]);
        allVertices.push_back(vertices[1]);
        allVertices.push_back(vertices[2]);
        allVertices.push_back(vertices[3]);

        uint32_t offset = i * 4;
        std::vector<uint32_t> indices = {0 + offset, 1 + offset, 2 + offset, 2 + offset, 3 + offset, 0 + offset};
        for (uint32_t index : indices) {
            allIndices.push_back(index);
        }
    }

    WvkModel *model = new WvkModel(device, allVertices, allIndices);
    models.push_back(model);*/

    //WvkModel *model = new WvkModel(device, "viking_room.obj.model");
    //models.push_back(model);

    WvkModel *model = new WvkModel(device, "cube.obj.model");
    models.push_back(model);
}

}
