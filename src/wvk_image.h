#pragma once

#include "wvk_device.h"
#include "resource_path.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace wvk {

class Image {
public:
    Image() {}
    Image(WvkDevice& device, std::string filename);
    void cleanup();

    VkDevice device = VK_NULL_HANDLE;

    uint32_t width;
    uint32_t height;
    uint32_t channels;

    VkImage image;
    VkDeviceMemory imageMemory;
    VkImageView imageView;
};

}
