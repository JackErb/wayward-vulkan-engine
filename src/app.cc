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
    createPipelineResources();
    logger::debug("Created pipeline resources");

    createPipelines();
    logger::debug("Created application pipelines");

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

    for (auto &image : textureImages) {
        image.cleanup();
    }

    for (auto &buffer : cameraTransformBuffers) {
        buffer.cleanup();
    }

    textureSampler.cleanup();

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

void WvkApplication::createPipelineResources() {
    // Allocate texture images
    for (size_t i = 0; i < images.size(); i++) {
        textureImages.push_back(Image{device, images[i]});
    }

    logger::debug("uniform buffers");
    // Allocate uniform buffers (one per swapchain image)
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    for (size_t i = 0; i < swapChain.getImageCount(); i++) {
        cameraTransformBuffers.emplace_back();
        device.createBuffer(bufferSize,
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            cameraTransformBuffers[i]);
    }
}

void WvkApplication::createPipelines() {
    PipelineConfigInfo defaultConfig = WvkPipeline::defaultPipelineConfigInfo();

    DescriptorSetInfo mainDescriptor{};
    auto &mainLayout = mainDescriptor.layoutBindings;

    mainLayout.resize(3);

    mainLayout[0].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    mainLayout[0].count = textureImages.size();
    mainLayout[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    mainLayout[0].unique = false;
    for (size_t i = 0; i < textureImages.size(); i++) {
        mainLayout[0].data[0][i].imageView = textureImages[i].imageView;
    }

    mainLayout[1].type = VK_DESCRIPTOR_TYPE_SAMPLER;
    mainLayout[1].count = 1;
    mainLayout[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    mainLayout[1].unique = false;
    mainLayout[1].data[0][0].sampler = textureSampler.sampler;

    mainLayout[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    mainLayout[2].count = 1;
    mainLayout[2].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    mainLayout[2].unique = true;
    for (size_t i = 0; i < swapChain.getImageCount(); i++) {
        logger::debug(cameraTransformBuffers[i].buffer);
        mainLayout[2].data[i][0].buffer = cameraTransformBuffers[i].buffer;
    }

    pipeline = std::make_unique<WvkPipeline>(device, swapChain, swapChain.getRenderPass(),
                                             "triangle.vert.spv", "triangle.frag.spv",
                                             mainDescriptor,
                                             WvkPipeline::defaultPipelineConfigInfo());

    shadowPipeline = std::make_unique<WvkPipeline>(device, swapChain, swapChain.getShadowRenderPass(),
                                             "triangle.vert.spv", "",
                                             mainDescriptor,
                                             WvkPipeline::defaultPipelineConfigInfo());
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

void WvkApplication::updateCamera(int imageIndex, VkExtent2D extent, float offset) {
    float rotation = (float) frame / 1000.f;

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), rotation * glm::radians(90.f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f + offset), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.projection = glm::perspective(glm::radians(45.0f), (float) extent.width / (float) extent.height, 0.1f, 10.0f);
    ubo.projection[1][1] *= -1;

    void *pData;
    vkMapMemory(device.getDevice(), cameraTransformBuffers[imageIndex].memory, 0, sizeof(ubo), 0, &pData);
    memcpy(pData, &ubo, sizeof(ubo));
    vkUnmapMemory(device.getDevice(), cameraTransformBuffers[imageIndex].memory);
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

    updateCamera(imageIndex, extent, -0.4f);
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

    updateCamera(imageIndex, extent, -0.f);
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

    frame++;

    recordShadowRenderPass(imageIndex);
    recordMainRenderPass(imageIndex);

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
