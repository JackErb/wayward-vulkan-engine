#pragma once

#include "wvk_device.h"
#include "resource_path.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace wvk {

class WvkImage {
public:
    WvkImage(WvkDevice& device, std::string filename);
    ~WvkImage();

    uint32_t getWidth() { return width; }
    uint32_t getHeight() { return height; }

    VkImage getImage() { return image; }
    VkImageView getImageView() { return imageView; }

private:
    WvkDevice& device;

    uint32_t width;
    uint32_t height;
    uint32_t channels;

    VkImage image;
    VkDeviceMemory imageMemory;
    VkImageView imageView;
};

}
