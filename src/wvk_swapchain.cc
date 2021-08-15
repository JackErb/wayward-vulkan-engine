#include "wvk_swapchain.h"

#include "wvk_device.h"
#include "wvk_helper.h"
#include <logger.h>

#include <algorithm>
#include <array>

namespace wvk {

WvkSwapchain::WvkSwapchain(WvkDevice &device, VkExtent2D extent) : device{device}, windowExtent{extent} {
    cacheDeviceProperties();

    createSwapchain();
    logger::debug("Created swapchain");

    createSwapchainImages();
    logger::debug("Created swapchain images & image views");

    createDepthResources();
    logger::debug("Created depth resources");

    createShadowRenderPass();
    createRenderPass();
    logger::debug("Created render pass");

    createSwapchainFramebuffers();
    logger::debug("Created swapchain framebuffers");

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

    vkDestroyImageView(dev, shadowDepthImageView, nullptr);
    vkDestroyImage(dev, shadowDepthImage, nullptr);
    vkFreeMemory(dev, shadowDepthImageMemory, nullptr);

    vkDestroyImageView(dev, shadowImageView, nullptr);
    vkDestroyImage(dev, shadowImage, nullptr);
    vkFreeMemory(dev, shadowImageMemory, nullptr);

    vkDestroyImageView(dev, colorImageView, nullptr);
    vkDestroyImage(dev, colorImage, nullptr);
    vkFreeMemory(dev, colorImageMemory, nullptr);

    vkDestroyImageView(dev, colorDepthImageView, nullptr);
    vkDestroyImage(dev, colorDepthImage, nullptr);
    vkFreeMemory(dev, colorDepthImageMemory, nullptr);

    vkDestroyRenderPass(dev, renderPass, nullptr);
    for (auto framebuffer : framebuffers) {
        vkDestroyFramebuffer(dev, framebuffer, nullptr);
    }

    vkDestroyRenderPass(dev, shadowRenderPass, nullptr);
    vkDestroyFramebuffer(dev, shadowFramebuffer, nullptr);

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

void WvkSwapchain::createDepthResources() {
    device.createImage(swapChainExtent.width, swapChainExtent.height,
                       imageFormat, VK_IMAGE_TILING_OPTIMAL,
                       samples,
                       VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                       colorImage, colorImageMemory);

    colorImageView = device.createImageView(colorImage, imageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

    device.createImage(swapChainExtent.width, swapChainExtent.height,
                       VK_FORMAT_D32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
                       samples,
                       VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                       colorDepthImage, colorDepthImageMemory);

    colorDepthImageView = device.createImageView(colorDepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

    device.createImage(swapChainExtent.width, swapChainExtent.height,
                       imageFormat, VK_IMAGE_TILING_OPTIMAL,
                       VK_SAMPLE_COUNT_1_BIT,
                       VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                       shadowImage, shadowImageMemory);

    shadowImageView = device.createImageView(shadowImage, imageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

    device.createImage(swapChainExtent.width, swapChainExtent.height,
                       VK_FORMAT_D32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
                       VK_SAMPLE_COUNT_1_BIT,
                       VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                       shadowDepthImage, shadowDepthImageMemory);

    shadowDepthImageView = device.createImageView(shadowDepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void WvkSwapchain::createShadowRenderPass() {
    VkAttachmentDescription shadowColorAttachment{};
    shadowColorAttachment.format = imageFormat;
    shadowColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    shadowColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    shadowColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    shadowColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    shadowColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    shadowColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    shadowColorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference shadowColorAttachmentRef;
    shadowColorAttachmentRef.attachment = 0;
    shadowColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription shadowDepthAttachment{};
    shadowDepthAttachment.format = depthFormat;
    shadowDepthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    shadowDepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    shadowDepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    shadowDepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    shadowDepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    shadowDepthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    shadowDepthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    VkAttachmentReference shadowDepthAttachmentRef;
    shadowDepthAttachmentRef.attachment = 1;
    shadowDepthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &shadowColorAttachmentRef;
    subpass.pDepthStencilAttachment = &shadowDepthAttachmentRef;
    subpass.pResolveAttachments = nullptr;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {shadowColorAttachment, shadowDepthAttachment};
    std::array<VkSubpassDescription, 1> subpasses = {subpass};
    std::array<VkSubpassDependency, 1> dependencies = {dependency};

    VkRenderPassCreateInfo renderInfo{};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderInfo.pAttachments = attachments.data();
    renderInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
    renderInfo.pSubpasses = subpasses.data();
    renderInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderInfo.pDependencies = dependencies.data();

    VkResult result = vkCreateRenderPass(device.getDevice(), &renderInfo, nullptr, &shadowRenderPass);
    checkVulkanError(result, "failed to create shadow render pass");
}

void WvkSwapchain::createRenderPass() {
    // Main color attachment
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = imageFormat;
    colorAttachment.samples = samples;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef;
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Main depth attachment
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = depthFormat;
    depthAttachment.samples = samples;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef;
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Framebuffer attachment
    VkAttachmentDescription framebufferAttachment{};
    framebufferAttachment.format = imageFormat;
    framebufferAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    framebufferAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    framebufferAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    framebufferAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    framebufferAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    framebufferAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    framebufferAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference framebufferAttachmentRef;
    framebufferAttachmentRef.attachment = 2;
    framebufferAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    subpass.pResolveAttachments = &framebufferAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask =  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, framebufferAttachment};
    std::array<VkSubpassDescription, 1> subpasses = {subpass};
    std::array<VkSubpassDependency, 1> dependencies = {dependency};

    VkRenderPassCreateInfo renderInfo{};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderInfo.pAttachments = attachments.data();
    renderInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
    renderInfo.pSubpasses = subpasses.data();
    renderInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderInfo.pDependencies = dependencies.data();

    VkResult result = vkCreateRenderPass(device.getDevice(), &renderInfo, nullptr, &renderPass);
    checkVulkanError(result, "failed to create render pass");
}

void WvkSwapchain::createSwapchainFramebuffers() {
    for (VkImageView imageView : imageViews) {
        std::array<VkImageView, 3> attachments = {
            colorImageView,
            colorDepthImageView,
            imageView
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.flags = 0;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        VkFramebuffer framebuffer;
        VkResult result = vkCreateFramebuffer(device.getDevice(), &framebufferInfo, nullptr, &framebuffer);
        checkVulkanError(result, "failed to create swap chain framebuffer");

        framebuffers.push_back(framebuffer);
    }

    std::array<VkImageView, 2> shadowAttachments = {shadowImageView, shadowDepthImageView};

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.flags = 0;
    framebufferInfo.renderPass = shadowRenderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(shadowAttachments.size());
    framebufferInfo.pAttachments = shadowAttachments.data();
    framebufferInfo.width = swapChainExtent.width;
    framebufferInfo.height = swapChainExtent.height;
    framebufferInfo.layers = 1;

    VkResult result = vkCreateFramebuffer(device.getDevice(), &framebufferInfo, nullptr, &shadowFramebuffer);
    checkVulkanError(result, "failed to create shadow frame buffer");
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
