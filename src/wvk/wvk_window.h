#pragma once

#include <logger.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <vector>

namespace wvk {

class WvkWindow {
  public:
    WvkWindow(int width, int height, std::string name);
    ~WvkWindow();

    WvkWindow(const WvkWindow &) = delete;
    WvkWindow &operator=(const WvkWindow &) = delete;

    bool shouldClose() { return glfwWindowShouldClose(window); }
    VkExtent2D getExtent() { return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}; }

    void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

  private:
    void initWindow();

    const int width;
    const int height;
    std::string name;

    GLFWwindow *window;
};

};
