#include "app.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <logger.h>
#include <thread>
#include <chrono>

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::microseconds;

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
    vkQueueWaitIdle(present_queue);

    current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void application::run() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        draw_frame();
        //std::this_thread::sleep_for(milliseconds(16));
    }

    vkDeviceWaitIdle(device);

    glfwDestroyWindow(window);

    glfwTerminate();

    return;
}
