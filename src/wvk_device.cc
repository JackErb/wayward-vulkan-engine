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
    logger::debug("Created surface");
    pickPhysicalDevice();
    logger::debug("Found suitable physical device");
    createLogicalDevice();
    logger::debug("Created logical device");
    createCommandPool();
    logger::debug("Created command pool");
}

WvkDevice::~WvkDevice() {
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyDevice(device, nullptr);
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
            this->physicalDevice = device;
            this->queueIndices = deviceIndices;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        logger::fatal_error("failed to find suitable physical device");
    } else {
        // Cache some information about the physical device
        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
    }
}

/* Creating the logical device */

void WvkDevice::createLogicalDevice() {
    // Create queues
    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    std::set<uint32_t> queueFamilies = {queueIndices.graphicsQueue, queueIndices.presentQueue};
    float queuePriority = 1.f;
    for (uint32_t queueFamily : queueFamilies) {
        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = queueFamily;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &queuePriority;

        queueInfos.push_back(queueInfo);
    }

    // Required extensions
    std::vector<const char*> extensions = getRequiredDeviceExtensions(physicalDevice);

    // Device features
    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    // Create device
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = queueInfos.size();
    createInfo.pQueueCreateInfos = queueInfos.data();
    createInfo.enabledExtensionCount = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.pEnabledFeatures = &deviceFeatures;

    // Set validation layers
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = requiredLayers.size();
        createInfo.ppEnabledLayerNames = requiredLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
    checkVulkanError(result, "failed to create logical device");

    vkGetDeviceQueue(device, queueIndices.graphicsQueue, 0, &graphicsQueue);
    vkGetDeviceQueue(device, queueIndices.presentQueue, 0, &presentQueue);
}

uint32_t WvkDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        bool suitableType = typeFilter & (1 << i);
        bool suitableProps = (memProperties.memoryTypes[i].propertyFlags & properties) == properties;
        if (suitableType && suitableProps) {
            return i;
        }
    }

    logger::fatal_error("failed to find suitable memory type.");
}

void WvkDevice::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                             Buffer &buffer) {
    buffer.device = device;
    buffer.size = size;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &buffer.buffer);
    checkVulkanError(result, "failed to create buffer");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer.buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    result = vkAllocateMemory(device, &allocInfo, nullptr, &buffer.memory);
    checkVulkanError(result, "failed to allocate buffer device memory");

    vkBindBufferMemory(device, buffer.buffer, buffer.memory, 0);
}

void WvkDevice::createImage(uint32_t width, uint32_t height,
                      VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                      VkImage &image, VkDeviceMemory &imageMemory) {
    // Create VkImage
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.flags = 0;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = format;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = tiling;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkResult result = vkCreateImage(device, &imageInfo, nullptr, &image);
    checkVulkanError(result, "failed to create image.");

    // Create device memory for texture image

    // Get memory requirements for newly created image
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    // Allocate device memory
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    result = vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory);
    checkVulkanError(result, "failed to allocate device memory.");

    // Bind the device memory to the image
    vkBindImageMemory(device, image, imageMemory, 0);
}

VkImageView WvkDevice::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;

    // TODO: unnecessary ?
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    VkResult result = vkCreateImageView(device, &viewInfo, NULL, &imageView);
    checkVulkanError(result, "failed to create image view.");

    return imageView;
}

void WvkDevice::createCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueIndices.graphicsQueue;

    VkResult result = vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
    checkVulkanError(result, "failed to create command pool");
}

VkCommandBuffer WvkDevice::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void WvkDevice::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    VkResult result = vkEndCommandBuffer(commandBuffer);
    checkVulkanError(result, "single time commands failed to end");

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void WvkDevice::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    endSingleTimeCommands(commandBuffer);
}

void WvkDevice::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferImageCopy bufferCopy{};
    bufferCopy.bufferOffset = 0;
    bufferCopy.bufferRowLength = width;
    bufferCopy.bufferImageHeight = height;
    bufferCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bufferCopy.imageSubresource.mipLevel = 0;
    bufferCopy.imageSubresource.baseArrayLayer = 0;
    bufferCopy.imageSubresource.layerCount = 1;
    bufferCopy.imageOffset = {0, 0, 0};
    bufferCopy.imageExtent = {
        width,
        height,
        1
    };

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &bufferCopy
    );

    endSingleTimeCommands(commandBuffer);
}

void WvkDevice::copyBuffer(Buffer src, Buffer dst, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy bufferCopy{};
    bufferCopy.srcOffset = 0;
    bufferCopy.dstOffset = 0;
    bufferCopy.size = size;

    vkCmdCopyBuffer(commandBuffer, src.buffer, dst.buffer, 1, &bufferCopy);

    endSingleTimeCommands(commandBuffer);
}

}
