#ifndef APPLICATION_H
#define APPLICATION_H

#include "attributes.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <optional>
#include <vector>

static std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f, 0.f}, {0.f, 0.f}},
    {{-0.5f, 0.5f, 0.f}, {0.f, 1.f}},
    {{0.5f, 0.5f, 0.f}, {1.f, 1.f}},
    {{0.5f, -0.5f, 0.f}, {1.f, 0.f}},

    {{-0.5f, -0.5f, -0.5f}, {0.f, 0.f}},
    {{-0.5f, 0.5f, -0.5f}, {0.f, 1.f}},
    {{0.5f, 0.5f, -0.5f}, {1.f, 1.f}},
    {{0.5f, -0.5f, -0.5f}, {1.f, 0.f}},
};

static std::vector<uint16_t> vertex_indices = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
};
  

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
    void draw_frame();
    void update_uniform_buffer(int imageIndex);

    /* Initialization */
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
    void create_descriptor_set_layout();
    void create_framebuffers();
    void create_command_pool();
    void create_depth_resources();
    void create_image(uint32_t width, uint32_t height, VkFormat format,
                      VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                      VkImage& image, VkDeviceMemory& imageMemory);
    void create_texture_image();
    void create_texture_image_view();
    void create_texture_sampler();
    void create_vertex_buffer();
    void create_index_buffer();
    void create_uniform_buffers();
    void create_descriptor_pool();
    void create_descriptor_sets();
    void create_command_buffers();
    void create_synchronization_objects();

    VkSurfaceFormatKHR choose_swap_surface_format();
    VkPresentModeKHR choose_swap_present_mode();
    VkExtent2D choose_swap_extent();

    bool is_device_suitable(VkPhysicalDevice device);
    QueueIndices find_queue_families(VkPhysicalDevice device);
    void query_swap_chain_support(VkPhysicalDevice device);
    VkShaderModule create_shader_module(const std::string& filename);
    void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory);

    VkImageView create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

    void update_vertex_buffer(std::vector<Vertex> data);
    void copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
    uint32_t find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags memoryPropertes);

    VkCommandBuffer begin_single_time_commands();
    void end_single_time_commands(VkCommandBuffer commandBuffer);

    void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    void transition_image_layout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

    GLFWwindow* window = nullptr;

    // The Vulkan instance
    VkInstance instance = VK_NULL_HANDLE;

    // The physical device (GPU) to be utilized
    VkPhysicalDeviceProperties physical_device_properties;
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
 
    VkImage depth_image;
    VkDeviceMemory depth_image_memory;
    VkImageView depth_image_view;

    // The graphics pipeline
    VkRenderPass render_pass;
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;

    // Commander buffers
    VkCommandPool command_pool;
    std::vector<VkCommandBuffer> command_buffers;

    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;

    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;

    VkBuffer index_buffer;
    VkDeviceMemory index_buffer_memory;

    std::vector<VkBuffer> uniform_buffers;
    std::vector<VkDeviceMemory> uniform_buffers_memory;

    VkDescriptorPool descriptor_pool;
    std::vector<VkDescriptorSet> descriptor_sets;

    VkImage texture_image;
    VkDeviceMemory texture_image_memory;

    VkImageView texture_image_view;
    VkSampler texture_sampler;

    // Synchronization
    const int MAX_FRAMES_IN_FLIGHT = 2;
    size_t current_frame = 0;
    std::vector<VkSemaphore> image_available_semaphores;
    std::vector<VkSemaphore> render_finished_semaphores;
    std::vector<VkFence> in_flight_fences;
    std::vector<VkFence> images_in_flight;

    // Cached data from querying device properties
    QueueIndices indices;
    SwapChainSupportDetails swap_chain_details;
};

#endif
