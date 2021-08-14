#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "wvk_device.h"

namespace wvk {

struct Sampler {
    Sampler(WvkDevice &device, VkSamplerAddressMode addressMode);
    void cleanup();

    VkSampler sampler;
    VkDevice device;
};

}
