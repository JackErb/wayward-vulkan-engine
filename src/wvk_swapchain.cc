#include "wvk_swapchain.h"

#include "wvk_device.h"
#include "wvk_helper.h"
#include <logger.h>

#include <algorithm>
#include <array>

namespace wvk {

WvkSwapchain::WvkSwapchain(WvkDevice &device, VkExtent2D extent) : device{device}, windowExtent{extent},
                                                                   shadowRenderPass{device, *this}, mainRenderPass{device, *this} {
    cacheDeviceProperties();

    createSwapchain();
    logger::debug("Created swapchain");

    createSwapchainImages();
    logger::debug("Created swapchain images & image views");

    createRenderPasses();
    logger::debug("Created render passes");

    createSynchronizationObjects();
    logger::debug("Created swap chain semaphores & fences");
}

WvkSwapchain::~WvkSwapchain() {
    VkDevice dev = device.getDevice();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(dev, imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(dev, renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(dev, inFlightFences[i], nullptr);
    }

    for (auto imageView : imageViews) {
        vkDestroyImageView(dev, imageView, nullptr);
    }
    vkDestroySwapchainKHR(dev, swapChain, nullptr);
}

void WvkSwapchain::cacheDeviceProperties() {
    PhysicalDeviceProperties props = device.getPhysicalDeviceProperties();

    static const VkSampleCountFlagBits DESIRED_SAMPLES = VK_SAMPLE_COUNT_4_BIT;
    VkSampleCountFlagBits maxSamples = props.maxSampleCount;

    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    if (maxSamples >= DESIRED_SAMPLES) {
        this->samples = DESIRED_SAMPLES;
    }
}

SwapchainSupportDetails WvkSwapchain::querySwapchainSupport() {
    SwapchainSupportDetails details;

    VkPhysicalDevice physicalDevice = device.getPhysicalDevice();
    VkSurfaceKHR surface = device.getSurface();

    // Get surface capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

    // Get surface formats
    uint32_t formatsCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatsCount, nullptr);
    if (formatsCount > 0) {
        details.surfaceFormats.resize(formatsCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatsCount, details.surfaceFormats.data());
    }

    // Get surface present modes
    uint32_t presentModesCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModesCount, nullptr);
    if (presentModesCount > 0) {
        details.presentModes.resize(presentModesCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModesCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const SwapchainSupportDetails &details) {
    // Check if our preferred format is available
    for (const VkSurfaceFormatKHR& format : details.surfaceFormats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }

    // Default to the first format
    return details.surfaceFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const SwapchainSupportDetails &details) {
    // TODO: Investigate best present mode
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(const SwapchainSupportDetails &details, GLFWwindow *window) {
    VkSurfaceCapabilitiesKHR capabilities = details.capabilities;
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D min = capabilities.minImageExtent;
        VkExtent2D max = capabilities.maxImageExtent;

        VkExtent2D actualExtent{};
        actualExtent.width = std::clamp(static_cast<uint32_t>(width), min.width, max.width);
        actualExtent.height = std::clamp(static_cast<uint32_t>(height), min.height, max.height);

        return actualExtent;
    }
}

void WvkSwapchain::createSwapchain() {
    SwapchainSupportDetails swapchainDetails = querySwapchainSupport();

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainDetails);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainDetails);
    VkExtent2D extent = chooseSwapExtent(swapchainDetails, device.getWindow().getGlfwWindow());

    uint32_t imageCount = swapchainDetails.capabilities.minImageCount + 1;
    if (swapchainDetails.capabilities.maxImageCount != 0 && imageCount > swapchainDetails.capabilities.maxImageCount) {
        imageCount = swapchainDetails.capabilities.maxImageCount;
    }

    if (imageCount > MAX_FRAMES_IN_FLIGHT) {
        logger::debug("Image count is > MAX_FRAMES_IN_FLIGHT");
        imageCount = MAX_FRAMES_IN_FLIGHT;
    }

    // Create the swapchain
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.flags = 0;
    createInfo.surface = device.getSurface();
    createInfo.minImageCount = imageCount;

    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;

    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueIndices indices = device.getQueueIndices();
    uint32_t queueFamilyIndices[] = {indices.graphicsQueue, indices.presentQueue};
    if (indices.graphicsQueue != indices.presentQueue) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapchainDetails.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(device.getDevice(), &createInfo, nullptr, &swapChain);
    checkVulkanError(result, "failed to create swap chain");

    imageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void WvkSwapchain::createSwapchainImages() {
    // Get swap chain's VkImages
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(device.getDevice(), swapChain, &imageCount, nullptr);
    images.resize(imageCount);
    vkGetSwapchainImagesKHR(device.getDevice(), swapChain, &imageCount, images.data());

    // Make a VkImageView for each image
    for (VkImage image : images) {
        VkImageView imageView = device.createImageView(image, imageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
        imageViews.push_back(imageView);
    }
}

void WvkSwapchain::createRenderPasses() {
    // Shadow render pass
    ImageInfo shadowDepth{};
    shadowDepth.type = IMAGE_DEPTH;
    shadowDepth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    shadowDepth.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    shadowDepth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    shadowDepth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    shadowDepth.usage = VK_IMAGE_USAGE_SAMPLED_BIT;

    RenderPassInfo shadowPassInfo{};
    shadowPassInfo.images = {shadowDepth};
    shadowPassInfo.subpass.depthIndex = 0;

    auto attachments = shadowRenderPass.initRenderPass(shadowPassInfo);
    shadowRenderPass.createFramebuffer({attachments[0].view});

    shadowDepthAttachment = attachments[0];

    logger::debug("Created shadow render pass");

    // Main render pass
    ImageInfo mainColor{};
    mainColor.type = IMAGE_COLOR;
    mainColor.samples = samples;
    mainColor.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    ImageInfo mainDepth{};
    mainDepth.type = IMAGE_DEPTH;
    mainDepth.samples = samples;

    ImageInfo mainResolve{};
    mainResolve.type = IMAGE_COLOR;
    mainResolve.createImage = false;
    mainResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    mainResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    mainResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    mainResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    RenderPassInfo mainPassInfo{};
    mainPassInfo.images = {mainColor, mainDepth, mainResolve};
    mainPassInfo.subpass.colorIndex = 0;
    mainPassInfo.subpass.depthIndex = 1;
    mainPassInfo.subpass.resolveIndex = 2;

    attachments = mainRenderPass.initRenderPass(mainPassInfo);
    for (size_t i = 0; i < imageViews.size(); i++) {
        mainRenderPass.createFramebuffer({attachments[0].view, attachments[1].view, imageViews[i]});
    }

    logger::debug("Created main render pass");
}

void WvkSwapchain::createSynchronizationObjects() {
    int imageCount = getImageCount();
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight.resize(imageCount, VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkCreateSemaphore(device.getDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]);
        vkCreateSemaphore(device.getDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]);

        vkCreateFence(device.getDevice(), &fenceInfo, nullptr, &inFlightFences[i]);
    }
}

uint32_t WvkSwapchain::acquireNextImage() {
    VkDevice dev = device.getDevice();
    VkSemaphore imageAvailableSemaphore = imageAvailableSemaphores[currentFrame];
    VkFence inFlightFence = inFlightFences[currentFrame];

    vkWaitForFences(dev, 1, &inFlightFence, VK_TRUE, UINT64_MAX);

    // Acquire the next image from the swap chain
    uint32_t imageIndex;
    vkAcquireNextImageKHR(dev, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    // Check if image is currently in flight for this image
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(dev, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }

    imagesInFlight[imageIndex] = inFlightFence;

    return imageIndex;
}

void WvkSwapchain::submitCommands(VkCommandBuffer buffer, uint32_t imageIndex) {
    VkDevice dev = device.getDevice();
    VkSemaphore imageAvailableSemaphore = imageAvailableSemaphores[currentFrame];
    VkSemaphore renderFinishedSemaphore = renderFinishedSemaphores[currentFrame];
    VkFence inFlightFence = inFlightFences[currentFrame];

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &buffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(device.getDevice(), 1, &inFlightFence);

    VkResult result = vkQueueSubmit(device.getGraphicsQueue(), 1, &submitInfo, inFlightFence);
    checkVulkanError(result, "failed to submit draw command buffer to queue");

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(device.getPresentQueue(), &presentInfo);
    checkVulkanError(result, "failed to present frame (vkQueuePresentKHR)");

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

}
