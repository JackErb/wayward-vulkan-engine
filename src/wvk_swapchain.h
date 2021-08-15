#pragma once

#include "wvk_device.h"

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

    VkRenderPass getRenderPass() { return renderPass; }
    VkRenderPass getShadowRenderPass() { return shadowRenderPass; }
    VkFramebuffer getFramebuffer(size_t imageIndex) { return framebuffers[imageIndex]; }
    VkFramebuffer getShadowFramebuffer() { return shadowFramebuffer; }
    VkExtent2D getExtent() { return swapChainExtent; }
    uint32_t getImageCount() { return images.size(); }
    VkImageView getShadowDepthImageView() { return shadowDepthImageView; }

    uint32_t acquireNextImage();
    void submitCommands(VkCommandBuffer buffer, uint32_t imageIndex);

  private:
    void cacheDeviceProperties();

    void createSwapchain();

    void createSwapchainImages();
    void createDepthResources();
    void createSwapchainFramebuffers();

    void createShadowRenderPass();
    void createRenderPass();

    void createSynchronizationObjects();


    // helper functions
    SwapchainSupportDetails querySwapchainSupport();

    VkSwapchainKHR swapChain;

    // Device & presentation information
    VkFormat imageFormat;
    VkExtent2D swapChainExtent;
    VkExtent2D windowExtent;
    VkSampleCountFlagBits samples;

    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    std::vector<VkFramebuffer> framebuffers;

    VkRenderPass shadowRenderPass;
    VkRenderPass renderPass;

    // TODO: Choose this dynamically based on supported formats of physical device (vkGetPhysicalDeviceFormatProperties)
    const static VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

    VkImage colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;

    VkImage colorDepthImage;
    VkDeviceMemory colorDepthImageMemory;
    VkImageView colorDepthImageView;

    VkImage shadowImage;
    VkDeviceMemory shadowImageMemory;
    VkImageView shadowImageView;

    VkImage shadowDepthImage;
    VkDeviceMemory shadowDepthImageMemory;
    VkImageView shadowDepthImageView;
    VkFramebuffer shadowFramebuffer;

    WvkDevice &device;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;
};

}
