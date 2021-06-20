#ifndef APPLICATION_H
#define APPLICATION_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <optional>
#include <vector>

struct QueueIndices {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;

    bool has_queues() {
        return graphics.has_value() && present.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
};

class application {
public:
    application();
    ~application();

    void run();

private:
    void init_glfw();
    void init_vulkan();

    // Initialize vulkan and choose GPU
    void create_instance();
    void create_surface();
    void pick_physical_device();
    void create_logical_device();

    // Set up rendering infrastructure
    void create_swap_chain();
    void create_image_views();
    void create_render_pass();
    void create_graphics_pipeline();
    void create_framebuffers();

    VkSurfaceFormatKHR choose_swap_surface_format();
    VkPresentModeKHR choose_swap_present_mode();
    VkExtent2D choose_swap_extent();

    bool is_device_suitable(VkPhysicalDevice device);
    QueueIndices find_queue_families(VkPhysicalDevice device);
    void query_swap_chain_support(VkPhysicalDevice device);
    VkShaderModule create_shader_module(const std::string& filename);

    GLFWwindow* window = nullptr;

    // The Vulkan instance
    VkInstance instance = VK_NULL_HANDLE;

    // The physical device (GPU) to be utilized
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;

    // The logical device, a specific instance of the physical device
    VkDevice device = VK_NULL_HANDLE;

    // Queue handles
    VkQueue graphics_queue;
    VkQueue present_queue;

    // The window surface
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    // The swap chain
    VkSwapchainKHR swap_chain;
    std::vector<VkImage> swap_chain_images;
    std::vector<VkImageView> swap_chain_image_views;
    std::vector<VkFramebuffer> swap_chain_framebuffers;

    VkFormat swap_chain_format;
    VkExtent2D swap_chain_extent;
    
    // The graphics pipeline
    VkRenderPass render_pass;
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;

    // Cached data from querying device properties
    QueueIndices indices;
    SwapChainSupportDetails swap_chain_details;
};

#endif
