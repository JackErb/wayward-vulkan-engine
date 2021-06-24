#include "wvk_window.h"
#include "wvk_helper.h"

#include <logger.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <vector>

namespace wvk {

WvkWindow::WvkWindow(int width, int height, std::string name) : width{width}, height{height}, name{name} {
    initWindow();
}

WvkWindow::~WvkWindow() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

void WvkWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR *surface) {
    VkResult result = glfwCreateWindowSurface(instance, window, nullptr, surface);
    checkVulkanError(result, "failed to create window surface");
}

void WvkWindow::initWindow() {
    if (!glfwInit()) {
        logger::fatal_error("failed to initialize glfw");
    }

    if (!glfwVulkanSupported()) {
        logger::fatal_error("this device does not support vulkan");
    }

    // Create the glfw window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(width, height, "Vulkan window", nullptr, nullptr);
}

};
