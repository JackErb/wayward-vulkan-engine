#include "app.h"

#include "attributes.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <logger.h>
#include <thread>
#include <chrono>
#include <vector>
#include <array>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std::chrono;

static int frame = 0;

void application::update_uniform_buffer(int imageIndex) {
    float rotation = (float) frame / 1000.f;

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), rotation * glm::radians(90.f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), (float) swap_chain_extent.width / (float) swap_chain_extent.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;

    void *pData;
    vkMapMemory(device, uniform_buffers_memory[imageIndex], 0, sizeof(ubo), 0, &pData);
    memcpy(pData, &ubo, sizeof(ubo));
    vkUnmapMemory(device, uniform_buffers_memory[imageIndex]);
}

void application::draw_frame() {
    VkSemaphore image_available_semaphore = image_available_semaphores[current_frame];
    VkSemaphore render_finished_semaphore = render_finished_semaphores[current_frame];
    VkFence in_flight_fence = in_flight_fences[current_frame];

    vkWaitForFences(device, 1, &in_flight_fence, VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(device, swap_chain, UINT64_MAX, image_available_semaphore, VK_NULL_HANDLE, &imageIndex);

    // Check if a previous frame is using this image (i.e. there is its fence to wait on)
    if (images_in_flight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device, 1, &images_in_flight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    // Mark the image as now being in use by this frame
    images_in_flight[imageIndex] = in_flight_fence;
    
    update_vertex_buffer(vertices);
    update_uniform_buffer(imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {image_available_semaphore};
    VkSemaphore signalSemaphores[] = {render_finished_semaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &command_buffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(device, 1, &in_flight_fence);
    VkResult result = vkQueueSubmit(graphics_queue, 1, &submitInfo, in_flight_fence);
    if (result != VK_SUCCESS) {
        logger::fatal_error("failed to submit command buffer to the graphics queue. error code: " + std::to_string(result));
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapChains[] = {swap_chain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    vkQueuePresentKHR(present_queue, &presentInfo);
    //vkQueueWaitIdle(present_queue);

    current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

time_point<high_resolution_clock> get_time() {
    return high_resolution_clock::now();
}

void application::run() {
    frame = 0;

    long msCount = 0;
    const int FRAME_INTERVAL = 900;
    while (!glfwWindowShouldClose(window)) {
        auto start = get_time();

        glfwPollEvents();
        draw_frame();

        frame++;
     
        auto now = get_time();
        msCount += duration_cast<microseconds>(now - start).count();
        if (frame % FRAME_INTERVAL == 0) {
            float avg = ((float)msCount / 1000.f) / (float)FRAME_INTERVAL;
            logger::debug("average frame time: " + std::to_string(avg) + " ms");
            msCount = 0;
        }

        //std::this_thread::sleep_for(milliseconds(16));
    }

    vkDeviceWaitIdle(device);

    glfwDestroyWindow(window);

    glfwTerminate();

    return;
}
