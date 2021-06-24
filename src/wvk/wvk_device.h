#pragma once

#include "wvk_window.h"
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

  private:
    void createInstance();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();

    // Helper functions
    bool isDeviceSuitable(VkPhysicalDevice device, QueueIndices *indices);

    WvkWindow &window;
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;

    QueueIndices queueIndices;
};

};
