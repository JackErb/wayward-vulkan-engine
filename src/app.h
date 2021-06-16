#ifndef APPLICATION_H
#define APPLICATION_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <optional>

struct QueueIndices {
    std::optional<uint32_t> graphics;
};

class application {
public:
    application();
    ~application();

    void run();

private:
    void create_instance();
    void create_surface();
    void pick_physical_device();
    void create_logical_device();

    GLFWwindow* window;
 
    // The Vulkan instance
    VkInstance instance;

    // The physical device (GPU) to be utilized
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    
    // The logical device, a specific instance of the physical device
    VkDevice device = VK_NULL_HANDLE;
    
    // Queue handles
    QueueIndices indices;
    VkQueue graphics_queue;

    // The window surface
    VkSurfaceKHR surface;
};

#endif
