#include "app.h"

#include <SDL.h>
#include <logger.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <optional>

#include <cstring>

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::microseconds;

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

#define VULKAN_DEBUG_MESSAGES true

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif


std::vector<const char*>* get_instance_extensions() {
    // Get required extensions
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*>* extensions = new std::vector<const char*>();
    for (int i = 0; i < glfwExtensionCount; i++) {
        extensions->push_back(glfwExtensions[i]);
    }

    // TODO: This extension is only required if we are using the device extension VK_KHR_portability_subset
    const char* VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES2 = "VK_KHR_get_physical_device_properties2";
    extensions->push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES2);

    return extensions;
}

std::vector<const char*>* get_device_extensions(VkPhysicalDevice device) {
    std::vector<const char*>* extensions = new std::vector<const char*>();

    // Get device extensions
    uint32_t deviceExtensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, NULL, &deviceExtensionCount, NULL);

    std::vector<VkExtensionProperties> deviceExtensions(deviceExtensionCount);
    vkEnumerateDeviceExtensionProperties(device, NULL, &deviceExtensionCount, deviceExtensions.data());
    
    if (VULKAN_DEBUG_MESSAGES) {
        logger::debug("device extensions:");
        for (const auto& extension : deviceExtensions) {
            logger::debug(extension.extensionName);
        }
        logger::debug("---");
    }

    // Check if this extension is present. If it is, then we must enable it.
    const char* VK_KHR_PORTABILITY_SUBSET = "VK_KHR_portability_subset";
    for (const auto& extension : deviceExtensions) {
        if (strcmp(extension.extensionName, VK_KHR_PORTABILITY_SUBSET) == 0) {
            extensions->push_back(VK_KHR_PORTABILITY_SUBSET);
        }
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
    }

    for (const char* layerName : validationLayers) {
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

QueueIndices find_queue_families(VkPhysicalDevice device) {
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

        i++;
    }

    return indices;
}

bool is_device_suitable(const VkPhysicalDevice& device) {
    // TODO: Find most suitable device (discrete GPU, fallback to integrated)
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceProperties(device, &properties);
    vkGetPhysicalDeviceFeatures(device, &features);
    
    QueueIndices indices = find_queue_families(device);

    return indices.graphics.has_value();
}

application::application() {
    // Initialize glfw, and create the window
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan window", nullptr, nullptr);
    
    create_instance();
    create_surface();
    pick_physical_device();
    create_logical_device();
}

application::~application() {
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
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
    if (VULKAN_DEBUG_MESSAGES) {
        logger::debug("available extensions:");
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

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Set required extensions

    // TODO: Free this object at end of program
    std::vector<const char*>* instanceExtensions = get_instance_extensions();
    createInfo.enabledExtensionCount = instanceExtensions->size();
    createInfo.ppEnabledExtensionNames = instanceExtensions->data();

    createInfo.enabledLayerCount = 0;

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    // Create the global VkInstance
    auto result = vkCreateInstance(&createInfo, nullptr, &instance);
    if (result != VK_SUCCESS) {
        logger::error("failed to create vulkan instance!");
        logger::fatal_error("error code: " + std::to_string(result));
    }
}

void application::create_surface() {
    VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
    if (result != VK_SUCCESS) {
        logger::error("failed to create window surface");
        logger::fatal_error("error code: " + std::to_string(result));
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
    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = indices.graphics.value();
    queueInfo.queueCount = 1;

    float queuePriority = 1.0f;
    queueInfo.pQueuePriorities = &queuePriority;

    // Specify required physical device features
    VkPhysicalDeviceFeatures deviceFeatures{};

    // Create the logical device
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    // Set queues to be created
    createInfo.pQueueCreateInfos = &queueInfo;
    createInfo.queueCreateInfoCount = 1;
    
    // Set device features to be used
    createInfo.pEnabledFeatures = &deviceFeatures;

    // Set extensions to be used

    // TODO: Free this object at end of program
    std::vector<const char*>* deviceExtensions = get_device_extensions(physical_device);
    createInfo.enabledExtensionCount = deviceExtensions->size();
    createInfo.ppEnabledExtensionNames = deviceExtensions->data();

    // Set validation layers to be used
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    // Create the logical device
    VkResult result = vkCreateDevice(physical_device, &createInfo, nullptr, &device);
    
    if (result != VK_SUCCESS) {
        logger::error("unable to create logical device");
        logger::fatal_error("error code: " + std::to_string(result));
    }

    // Get a handle to the graphics queue
    vkGetDeviceQueue(device, indices.graphics.value(), 0, &graphics_queue);
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
