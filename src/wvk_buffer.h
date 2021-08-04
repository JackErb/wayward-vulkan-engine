#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace wvk {

// A VkBuffer backed by device memory
struct WvkBuffer {
    VkBuffer buffer;
    VkDeviceMemory memory;

    void cleanup(VkDevice device) {
        vkDestroyBuffer(device, buffer, nullptr);
        vkFreeMemory(device, memory, nullptr);
    }
};

}
