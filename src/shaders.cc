#include "app.h"

#include "resources.h"
#include <logger.h>

#include <fstream>

static std::vector<char> read_file(const std::string& filename) {
    std::string path = resourcePath();
    std::ifstream file(path + filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        logger::fatal_error("failed to open file: " + filename);
    }

    size_t fileSize = file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}

VkShaderModule application::create_shader_module(const std::string& filename) {
    std::vector<char> code = read_file(filename);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
    if (result != VK_SUCCESS) {
        logger::fatal_error("failed to create shader module. error code: " + std::to_string(result));
    }

    return shaderModule;
}
