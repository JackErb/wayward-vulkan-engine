#pragma once

#include "wvk_device.h"
#include "wvk_renderpass.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

namespace wvk {

class WvkDevice;

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    std::vector<VkPresentModeKHR> presentModes;
};

class WvkSwapchain {
  public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 3;

    WvkSwapchain(WvkDevice &device, VkExtent2D extent);
    ~WvkSwapchain();

    WvkSwapchain(const WvkSwapchain &) = delete;
    void operator=(const WvkSwapchain &) = delete;

    VkRenderPass getRenderPass() { return mainRenderPass.getRenderPass(); }
    VkRenderPass getShadowRenderPass() { return shadowRenderPass.getRenderPass(); }
    VkFramebuffer getFramebuffer(size_t imageIndex) { return mainRenderPass.getFramebuffer(imageIndex); }
    
    VkFramebuffer getShadowFramebuffer() { return shadowRenderPass.getFramebuffer(); }
    VkImageView getShadowDepthImageView() { return shadowDepthAttachment.view; }
    
    VkExtent2D getExtent() { return swapChainExtent; }
    uint32_t getImageCount() { return images.size(); }
    VkFormat getColorFormat() { return imageFormat; }
    VkFormat getDepthFormat() {
        // TODO: Choose this dynamically based on supported formats of physical device (vkGetPhysicalDeviceFormatProperties)
        return VK_FORMAT_D32_SFLOAT;
    }

    uint32_t acquireNextImage();
    void submitCommands(VkCommandBuffer buffer, uint32_t imageIndex);

  private:
    void cacheDeviceProperties();

    void createSwapchain();
    void createSwapchainImages();
    void createRenderPasses();
    void createSynchronizationObjects();


    // helper functions
    SwapchainSupportDetails querySwapchainSupport();

    VkSwapchainKHR swapChain;

    // Device & presentation information
    VkFormat imageFormat;
    VkExtent2D swapChainExtent;
    VkExtent2D windowExtent;
    VkSampleCountFlagBits samples;

    // Images belonging to the swapchain frame buffers
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    std::vector<VkFramebuffer> framebuffers;

    WvkRenderPass mainRenderPass;
    WvkRenderPass shadowRenderPass;

    Attachment shadowDepthAttachment;

    WvkDevice &device;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;
};

}
