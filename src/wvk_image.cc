#include "wvk_image.h"

#include <logger.h>

#include "stb_image.h"

namespace wvk {

Image::Image(WvkDevice& wvkDevice, std::string filename) : device{wvkDevice.getDevice()} {
    // Load pixel data
    std::string imagePath = resourcePath() + filename;
    int texWidth, texHeight, texChannels;

    stbi_uc* pixels = stbi_load(imagePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        logger::fatal_error("failed to load image file");
    }

    this->width = static_cast<uint32_t>(texWidth);
    this->height = static_cast<uint32_t>(texHeight);
    this->channels = static_cast<uint32_t>(texChannels);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    // Create staging buffer
    Buffer stagingBuffer;
    wvkDevice.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                           stagingBuffer);

    // Copy image data into staging buffer
    void *pData;
    vkMapMemory(device, stagingBuffer.memory, 0, imageSize, 0, &pData);
    memcpy(pData, pixels, static_cast<uint32_t>(imageSize));
    vkUnmapMemory(device, stagingBuffer.memory);
    logger::debug("Mapped memory");

    // Create the image & image view
    wvkDevice.createImage(width, height,
                          VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                          VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          image, imageMemory);
    imageView = wvkDevice.createImageView(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);

    // Copy image data from staging buffer to actual VkImage
    wvkDevice.transitionImageLayout(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    wvkDevice.copyBufferToImage(stagingBuffer.buffer, image, width, height);
    wvkDevice.transitionImageLayout(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    stagingBuffer.cleanup();

    logger::debug("Finished layout transitions");
}

void Image::cleanup() {
    if (device == VK_NULL_HANDLE) return;

    vkDestroyImageView(device, imageView, nullptr);
    vkDestroyImage(device, image, nullptr);
    vkFreeMemory(device, imageMemory, nullptr);
}

}
