#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

namespace wvk {


struct ShaderUniform {
    enum UniformType {
        Buffer, Image, Sampler
    };
};

class Shader {
public:
    Shader(std::string str);
    ~Shader();
};

};
