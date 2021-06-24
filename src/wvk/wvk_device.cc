#include "wvk_device.h"
#include "wvk_helper.h"

#include <string>
#include <set>
#include <vector>

std::vector<const char*> requiredLayers = {
    "VK_LAYER_KHRONOS_validation"
    //, "VK_LAYER_LUNARG_api_dump"
};

// If VK_KHR_portability_subset is a present device extension, then it must be enabled
const char* VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME = "VK_KHR_portability_subset";

std::vector<const char*> requiredDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

namespace wvk {

WvkDevice::WvkDevice(WvkWindow &window) : window{window} {
    createInstance();
    logger::debug("Created instance");
    window.createWindowSurface(instance, &surface);
    logger::debug("created surface");
    pickPhysicalDevice();
    createLogicalDevice();
}

WvkDevice::~WvkDevice() {
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}

/* Creating the instance */

bool checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> layers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

    std::set<std::string> requiredValidationLayers(requiredLayers.begin(), requiredLayers.end());
    for (VkLayerProperties layerProperties : layers) {
        requiredValidationLayers.erase(std::string(layerProperties.layerName));
    }

    bool hasValidationLayers = requiredValidationLayers.empty();
    if (!hasValidationLayers) {
        logger::error("missing validation layers:");
        for (std::string layer : requiredValidationLayers) {
            logger::error("    " + layer);
        }
    }

    return hasValidationLayers;
}

std::vector<const char*> getRequiredInstanceExtensions() {
    std::vector<const char*> extensions;

    // Get required glfw extensions
    uint32_t glfwExtensionsCount;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);

    for (size_t i = 0; i < glfwExtensionsCount; i++) {
        extensions.push_back(glfwExtensions[i]);
    }

    // Other required instance extensions

    // TODO: This should only be included if VK_KHR_portability_subset is a required device extension
    extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    return extensions;
}

void WvkDevice::createInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        logger::fatal_error("validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Wayward Vulkan";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Wayward Vulkan";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size());
        createInfo.ppEnabledLayerNames = requiredLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    std::vector<const char*> requiredExtensions = getRequiredInstanceExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    auto result = vkCreateInstance(&createInfo, nullptr, &instance);
    checkVulkanError(result, "failed to create vulkan instance");
}

/* Picking the physical device */

bool hasQueueFamilies(VkSurfaceKHR surface, VkPhysicalDevice device, QueueIndices *indices) {
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyProperties.data());

    bool hasGraphicsQueue = false;
    bool hasPresentQueue = false;

    for (size_t i = 0; i < queueFamilyProperties.size(); i++) {
        VkQueueFamilyProperties queueFamily = queueFamilyProperties[i];
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices->graphicsQueue = i;
            hasGraphicsQueue = true;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) {
            indices->presentQueue = i;
            hasPresentQueue = true;
        }
    }

    return hasGraphicsQueue && hasPresentQueue;
}

std::vector<VkExtensionProperties> getSupportedDeviceExtensions(VkPhysicalDevice device) {
    uint32_t extensionsCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, nullptr);
    
    std::vector<VkExtensionProperties> deviceExtensions(extensionsCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, deviceExtensions.data());
    return deviceExtensions;
}

std::vector<const char*> getRequiredDeviceExtensions(VkPhysicalDevice device) {
    std::vector<const char*> extensions;
    
    for (VkExtensionProperties extension : getSupportedDeviceExtensions(device)) {
        const char *extensionName = extension.extensionName;
        if (strcmp(extensionName, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME) == 0) {
            extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
        }
    }

    for (const char* extension : requiredDeviceExtensions) {
        extensions.push_back(extension);
    }

    return extensions;
}

bool hasRequiredExtensions(VkPhysicalDevice device) {
    std::vector<VkExtensionProperties> supportedExtensions = getSupportedDeviceExtensions(device);
    
    std::vector<const char*> requiredDeviceExtensions = getRequiredDeviceExtensions(device);
    std::set<std::string> requiredExtensions(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());
    for (VkExtensionProperties extension : supportedExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

bool WvkDevice::isDeviceSuitable(VkPhysicalDevice device, QueueIndices *indices) {
    // TODO: Query device features & properties to be more selective about GPU
    // Prefer discrete to integrated

    return hasRequiredExtensions(device) && hasQueueFamilies(surface, device, indices);
}

void WvkDevice::pickPhysicalDevice() {
    uint32_t deviceCount;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

    QueueIndices deviceIndices;

    for (VkPhysicalDevice device : physicalDevices) {
        if (isDeviceSuitable(device, &deviceIndices)) {
            logger::debug("Found suitable physical device.");
            this->physicalDevice = device;
            this->queueIndices = deviceIndices;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        logger::fatal_error("failed to find suitable physical device");
    }
}

/* Creating the logical device */

void WvkDevice::createLogicalDevice() {}

}
