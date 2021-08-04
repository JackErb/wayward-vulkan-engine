#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

namespace wvk {

std::vector<char> readFile(const std::string &filename);

VkShaderModule createShaderModule(filename);

};
