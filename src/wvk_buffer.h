#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace wvk {

// A VkBuffer backed by device memory
struct Buffer {
    Buffer() {}

    VkBuffer buffer;
    VkDeviceMemory memory;
    VkDeviceSize size;

    VkDevice device = VK_NULL_HANDLE;

    void cleanup() {
        if (device == VK_NULL_HANDLE) return;
        vkDestroyBuffer(device, buffer, nullptr);
        vkFreeMemory(device, memory, nullptr);
    }
};

}
