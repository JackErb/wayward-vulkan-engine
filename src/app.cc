#include "app.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <logger.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <string>
#include <vector>
#include <set>
#include <thread>
#include <chrono>
#include <optional>
#include <algorithm>

#include <cstring>
#include <cstdint>

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::microseconds;

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

#define VULKAN_DEBUG_MESSAGES false

/* Validation Layers */

const std::vector<const char*> requiredValidationLayers = {
    "VK_LAYER_KHRONOS_validation"
    //, "VK_LAYER_LUNARG_api_dump"
};

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif


/* Device extensions */

// If VK_KHR_portability_subset is a present device extension, then it must be enabled
const char* VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME = "VK_KHR_portability_subset";

const std::vector<const char*> requiredDeviceExtensions {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};


/* Helper Functions */

std::vector<const char*> get_instance_extensions() {
    // Get required extensions
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // The array of required extensions to be filled
    std::vector<const char*> extensions;

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    // TODO: This extension is only required if we are using the device extension VK_KHR_portability_subset
    extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    // Push the GLFW required extensions
    for (int i = 0; i < glfwExtensionCount; i++) {
        extensions.push_back(glfwExtensions[i]);
    }

    //extensions.push_back("VK_MVK_macos_surface");

    // Debug messages
    if (VULKAN_DEBUG_MESSAGES) {
        logger::debug("required instance extensions:");
        for (auto extension : extensions) {
            logger::debug(extension);
        }
        logger::debug("---");
    }

    return extensions;
}

std::vector<const char*> get_device_extensions(VkPhysicalDevice device) {
    std::vector<const char*> extensions;

    // Get device extensions
    uint32_t deviceExtensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, NULL, &deviceExtensionCount, NULL);

    std::vector<VkExtensionProperties> deviceExtensions(deviceExtensionCount);
    vkEnumerateDeviceExtensionProperties(device, NULL, &deviceExtensionCount, deviceExtensions.data());

    // Debug messages
    if (VULKAN_DEBUG_MESSAGES) {
        logger::debug("device extensions:");
        for (const auto& extension : deviceExtensions) {
            logger::debug(extension.extensionName);
        }
        logger::debug("---");
    }

    // Check if `VK_KHR_portability_subset` extension is present. If it is, then we must enable it.
    for (VkExtensionProperties extension : deviceExtensions) {
        if (strcmp(extension.extensionName, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME) == 0) {
            extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
        }
    }

    // Add required extensions
    for (const char* extension : requiredDeviceExtensions) {
        extensions.push_back(extension);
    }

    return extensions;
}

bool check_validation_layers() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    if (VULKAN_DEBUG_MESSAGES) {
        logger::debug("available layers:");
        for (const auto& layer : availableLayers) {
            logger::debug(layer.layerName);
        }
        logger::debug("---");

        logger::debug("required layers:");
        for (const char* layerName : requiredValidationLayers) {
            logger::debug(std::string(layerName));
        }
        logger::debug("---");
    }

    for (const char* layerName : requiredValidationLayers) {
        bool layerFound = false;
        for (const auto& layer : availableLayers) {
            if (strcmp(layerName, layer.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

void application::query_swap_chain_support(VkPhysicalDevice device) {
    // Get physics device capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &swap_chain_details.capabilities);

    // Get the formats our surface & physical device supports
    uint32_t formatsCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatsCount, nullptr);
    if (formatsCount != 0) {
        swap_chain_details.formats.resize(formatsCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatsCount, swap_chain_details.formats.data());
    }

    // Get the present modes available
    uint32_t presentsCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentsCount, nullptr);
    if (presentsCount != 0) {
        swap_chain_details.present_modes.resize(presentsCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentsCount, swap_chain_details.present_modes.data());
    }
}


/* Application functions */
bool application::is_device_suitable(VkPhysicalDevice device) {
    // TODO: Find most suitable device through ranking system (discrete GPU, fallback to integrated)
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceProperties(device, &properties);
    vkGetPhysicalDeviceFeatures(device, &features);

    /* Check queue support */
    QueueIndices indices = find_queue_families(device);
    bool hasRequiredQueues = indices.has_queues();

    /* Check extension support */
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);

    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, extensions.data());

    std::set<std::string> requiredExtensions(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());
    for (VkExtensionProperties extension : extensions) {
        requiredExtensions.erase(extension.extensionName);
    }
    bool hasRequiredExtensions = requiredExtensions.empty();

    /* Check swapchain support */
    bool swapChainAdequate = false;
    if (hasRequiredExtensions) {
        query_swap_chain_support(device);
        swapChainAdequate = !swap_chain_details.formats.empty() && !swap_chain_details.present_modes.empty();
    }

    return hasRequiredQueues && hasRequiredExtensions && swapChainAdequate;
}

application::application() {
    init_glfw();
    init_vulkan();
}

application::~application() {
    for (VkFramebuffer framebuffer : swap_chain_framebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    vkDestroyPipeline(device, graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    vkDestroyRenderPass(device, render_pass, nullptr);
    for (VkImageView imageView : swap_chain_image_views) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    vkDestroySwapchainKHR(device, swap_chain, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
}

void application::init_glfw() {
    // Initialize glfw
    if (!glfwInit()) {
        logger::fatal_error("failied to initialize glfw");
    }

    // Check if Vulkan is supported
    if (!glfwVulkanSupported()) {
        logger::fatal_error("this device does not support vulkan");
    }

    // Create the glfw window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan window", nullptr, nullptr);
 }

 void application::init_vulkan() {
    create_instance();

    create_surface();

    pick_physical_device();
    create_logical_device();

    create_swap_chain();
    create_image_views();

    create_render_pass();
    create_graphics_pipeline();
    create_framebuffers();
}

void application::create_instance() {
    // Check validation layer support
    if (enableValidationLayers && !check_validation_layers()) {
        logger::error("validation layers requested, but not available!");
    }

    // Get supported extensions
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    // Debug messages
    if (VULKAN_DEBUG_MESSAGES) {
        logger::debug("available instance extensions:");
        for (const auto& extension : extensions) {
            logger::debug(extension.extensionName);
        }
        logger::debug("---");
    }

    // Initialize the Vulkan application
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // Setup the VkInstanceCreateInfo
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Set required extensions
    std::vector<const char*> instanceExtensions = get_instance_extensions();

    createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();

    // Set validation layers
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(requiredValidationLayers.size());
        createInfo.ppEnabledLayerNames = requiredValidationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    // Create the global VkInstance
    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    if (result != VK_SUCCESS) {
        std::string msg = "failed to create vulkan instance. error code: " + std::to_string(result);
        logger::fatal_error(msg);
    }
}

void application::create_surface() {
    VkResult result = glfwCreateWindowSurface(instance, window, NULL, &surface);
    if (result != VK_SUCCESS) {
        std::string msg = "failed to create window surface. error code: " + std::to_string(result);
        logger::fatal_error(msg);
    }
}

void application::pick_physical_device() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
    if (deviceCount == 0) {
        logger::fatal_error("failed to find a GPU with Vulkan support");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& physicalDevice : devices) {
        if (is_device_suitable(physicalDevice)) {
            physical_device = physicalDevice;
            indices = find_queue_families(physical_device);
            if (VULKAN_DEBUG_MESSAGES) {
                logger::debug("found suitable phyical device");
                logger::debug("---");
            }
            break;
        }
    }

    if (physical_device == VK_NULL_HANDLE) {
        logger::fatal_error("failed to find a suitable GPU");
    }
}

void application::create_logical_device() {
    // Specify the commander buffer queues to be created
    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphics.value(), indices.present.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = queueFamily;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &queuePriority;

        queueInfos.push_back(queueInfo);
    }

    // Specify required physical device features
    VkPhysicalDeviceFeatures deviceFeatures{};

    // Create the logical device
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    // Set queues to be created
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
    createInfo.pQueueCreateInfos = queueInfos.data();

    // Set device features to be used
    createInfo.pEnabledFeatures = &deviceFeatures;

    // Set extensions to be used

    // TODO: Free this object at end of program
    std::vector<const char*> deviceExtensions = get_device_extensions(physical_device);
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    // Set validation layers to be used
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(requiredValidationLayers.size());
        createInfo.ppEnabledLayerNames = requiredValidationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    // Create the logical device
    VkResult result = vkCreateDevice(physical_device, &createInfo, nullptr, &device);

    if (result != VK_SUCCESS) {
        std::string msg = "unable to create logical device. error code: " + std::to_string(result);
        logger::fatal_error(msg);
    }

    // Get a handle to the graphics queue
    vkGetDeviceQueue(device, indices.graphics.value(), 0, &graphics_queue);
    vkGetDeviceQueue(device, indices.present.value(), 0, &present_queue);
}

QueueIndices application::find_queue_families(VkPhysicalDevice device) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

    std::vector<VkQueueFamilyProperties> properties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, properties.data());

    QueueIndices indices;

    int i = 0;
    for (const auto& property : properties) {
        if (property.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) {
            indices.present = i;
        }

        i++;
    }

    return indices;
}

void application::create_swap_chain() {
    query_swap_chain_support(physical_device);

    VkSurfaceFormatKHR surfaceFormat = choose_swap_surface_format();
    VkPresentModeKHR presentMode = choose_swap_present_mode();
    VkExtent2D extent = choose_swap_extent();

    uint32_t imageCount = swap_chain_details.capabilities.minImageCount + 1;
    if (swap_chain_details.capabilities.maxImageCount != 0 && imageCount > swap_chain_details.capabilities.maxImageCount) {
        imageCount = swap_chain_details.capabilities.maxImageCount;
    }

    /* Create the swapchain */
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;

    // Details of swap chain images
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;

    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = {indices.graphics.value(), indices.present.value()};
    if (indices.graphics != indices.present) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = swap_chain_details.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(device, &createInfo, nullptr, &swap_chain);
    if (result != VK_SUCCESS) {
        std::string msg = "unable to create swapchain. error code: " + std::to_string(result);
        logger::fatal_error(msg);
    }

    /* Retrieve the swapchain images */
    vkGetSwapchainImagesKHR(device, swap_chain, &imageCount, nullptr);
    swap_chain_images.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swap_chain, &imageCount, swap_chain_images.data());

    /* Cache other data about our swap chain */
    swap_chain_format = surfaceFormat.format;
    swap_chain_extent = extent;
}

VkSurfaceFormatKHR application::choose_swap_surface_format() {
    // Check if our preferred format is available
    for (const VkSurfaceFormatKHR& format : swap_chain_details.formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }

    // Default to the first format
    return swap_chain_details.formats[0];
}

VkPresentModeKHR application::choose_swap_present_mode() {
    // TODO: Investigate best present mode
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D application::choose_swap_extent() {
    VkSurfaceCapabilitiesKHR capabilities = swap_chain_details.capabilities;
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D min = swap_chain_details.capabilities.minImageExtent;
        VkExtent2D max = swap_chain_details.capabilities.maxImageExtent;

        VkExtent2D actualExtent{};
        actualExtent.width = std::clamp(static_cast<uint32_t>(width), min.width, max.width);
        actualExtent.height = std::clamp(static_cast<uint32_t>(height), min.height, max.height);

        return actualExtent;
    }
}

void application::create_image_views() {
    swap_chain_image_views.resize(swap_chain_images.size());

    for (size_t i = 0; i < swap_chain_images.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swap_chain_images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swap_chain_format;

        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        VkResult result = vkCreateImageView(device, &createInfo, NULL, &imageView);
        if (result != VK_SUCCESS) {
            logger::fatal_error("failed to create swap chain image views. error code: " + std::to_string(result));
        }

        swap_chain_image_views[i] = imageView;
    }
}

void application::create_render_pass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swap_chain_format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkResult result = vkCreateRenderPass(device, &renderPassInfo, nullptr, &render_pass);
    if (result != VK_SUCCESS) {
        logger::fatal_error("failed to create render pass. error code: " + std::to_string(result));
    }
}

void application::create_graphics_pipeline() {
    VkShaderModule vertexShader = create_shader_module("triangle.vert.spv");
    VkShaderModule fragShader = create_shader_module("triangle.frag.spv");

    /* Create the shader stages */
    VkPipelineShaderStageCreateInfo vertStageInfo{};
    vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStageInfo.module = vertexShader;
    vertStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragStageInfo{};
    fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStageInfo.module = fragShader;
    fragStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertStageInfo, fragStageInfo};

    /* Create the fixed functions */

    // Vertex input info
    VkPipelineVertexInputStateCreateInfo inputInfo{};
    inputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    inputInfo.vertexBindingDescriptionCount = 0;
    inputInfo.pVertexBindingDescriptions = nullptr;
    inputInfo.vertexAttributeDescriptionCount = 0;
    inputInfo.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport and scissors
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swap_chain_extent.width;
    viewport.height = (float) swap_chain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swap_chain_extent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // Set up rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Color blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipeline_layout);
    if (result != VK_SUCCESS) {
        logger::fatal_error("failed to create pipeline layout. error code: " + std::to_string(result));
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &inputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = pipeline_layout;
    pipelineInfo.renderPass = render_pass;
    pipelineInfo.subpass = 0;

    result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphics_pipeline);
    if (result != VK_SUCCESS) {
        logger::fatal_error("failed to create graphics pipeline. error code: " + std::to_string(result));
    }

    vkDestroyShaderModule(device, vertexShader, nullptr);
    vkDestroyShaderModule(device, fragShader, nullptr);
}

void application::create_framebuffers() {
    swap_chain_framebuffers.resize(swap_chain_image_views.size());
    for (size_t i = 0; i < swap_chain_image_views.size(); i++) {
        VkImageView attachments[] = {
            swap_chain_image_views[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = render_pass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swap_chain_extent.width;
        framebufferInfo.height = swap_chain_extent.height;
        framebufferInfo.layers = 1;

        VkResult result = vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swap_chain_framebuffers[i]);
        if (result != VK_SUCCESS) {
            logger::fatal_error("failed to create framebuffer. error code: " + std::to_string(result));
        }
    }
}

void application::run() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        std::this_thread::sleep_for(milliseconds(16));
    }

    glfwDestroyWindow(window);

    glfwTerminate();

    return;
}
