#pragma once

#include "wvk_window.h"
#include "wvk_swapchain.h"
#include "wvk_buffer.h"

#include <logger.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

namespace wvk {

struct QueueIndices {
   uint32_t graphicsQueue;
   uint32_t presentQueue;
};

class WvkDevice {
  public:
    /* VALIDATION LAYERS */

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif
    const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};

    /* DEVICE EXTENSIONS */
    const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    const char* VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME = "VK_KHR_portability_subset";

    /* CLASS DEFINITIONS */
    WvkDevice(WvkWindow &window);
    ~WvkDevice();

    WvkDevice(const WvkDevice &) = delete;
    WvkDevice &operator=(const WvkDevice &) = delete;
    WvkDevice(WvkDevice &&) = delete;
    WvkDevice &operator=(WvkDevice &&) = delete;

    VkInstance getInstance() { return instance; }
    WvkWindow &getWindow() { return window; }
    VkSurfaceKHR getSurface() { return surface; }
    VkPhysicalDevice getPhysicalDevice() { return physicalDevice; }
    VkDevice getDevice() { return device; }
    VkCommandPool getCommandPool() { return commandPool; }
    VkPhysicalDeviceProperties getPhysicalDeviceProprties() { return physicalDeviceProperties; }

    QueueIndices getQueueIndices() { return queueIndices; }
    VkQueue getGraphicsQueue() { return graphicsQueue; }
    VkQueue getPresentQueue() { return presentQueue; }


    void createBuffer(VkDeviceSize size,
                      VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties,
                      WvkBuffer &buffer);

    void createImage(uint32_t width,          uint32_t height,
                     VkFormat format,         VkImageTiling tiling,
                     VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                     VkImage &image,          VkDeviceMemory &imageMemory);
    VkImageView createImageView(VkImage image, VkFormat format,
                                VkImageAspectFlags aspectFlags);

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    void copyBuffer(WvkBuffer src, WvkBuffer dst, VkDeviceSize size);

    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

  private:
    void createInstance();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createCommandPool();

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    // Helper functions
    bool isDeviceSuitable(VkPhysicalDevice device, QueueIndices *indices);

    WvkWindow &window;
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkCommandPool commandPool;

    VkQueue graphicsQueue;
    VkQueue presentQueue;
    QueueIndices queueIndices;

    VkPhysicalDeviceProperties physicalDeviceProperties;
};

};
