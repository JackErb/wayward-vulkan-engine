#pragma once

#include <logger.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace wvk {

inline void checkVulkanError(VkResult result, const char *error) {
    if (result != VK_SUCCESS) {
        logger::fatal_error(error);
    }
}

}
