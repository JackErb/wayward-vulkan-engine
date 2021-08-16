#pragma once

#include "wvk_device.h"

#include <logger.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


namespace wvk {

class WvkSwapchain;

enum ImageType {
    IMAGE_COLOR, IMAGE_DEPTH
};

struct ImageInfo {
    ImageType type;
  
    bool createImage = true;
    std::vector<VkImage> images; // If createImage == false, this should be filled with the swapchain images.

    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    VkAttachmentLoadOp     loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkAttachmentStoreOp   storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    VkImageLayout   initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout     finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageUsageFlags       usage = 0;
};

struct SubpassInfo {
    // TODO: Add input attachment and preserve attachment info for when we support multiple subpasses
    
    // TODO: Support multiple of each attachment
    int colorIndex   = -1;
    int depthIndex   = -1;
    int resolveIndex = -1;
};

struct RenderPassInfo {
    std::vector<ImageInfo> images{};
    SubpassInfo subpass{};

    std::vector<VkImage> resolveImages{};
};

struct Attachment {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
};

class WvkRenderPass {
public:
    WvkRenderPass(WvkDevice &device, WvkSwapchain &swapchain) : device{device}, swapchain{swapchain} {};
    ~WvkRenderPass();

    WvkRenderPass(const WvkRenderPass &) = delete;
    WvkRenderPass &operator=(const WvkRenderPass &) = delete;

    VkRenderPass getRenderPass() { return renderPass; }

    VkFramebuffer getFramebuffer(int index = 0) {
        if (index >= framebuffers.size()) logger::fatal_error("invalid index WvkRenderPass::getFramebuffer()");
        return framebuffers[index];
    }
    
    std::vector<Attachment> initRenderPass(const RenderPassInfo &info);
    void createFramebuffer(std::vector<VkImageView> attachments);

private:
    std::vector<Attachment> createResources(const RenderPassInfo &);
    void createRenderPass(const RenderPassInfo &);

    std::vector<Attachment> images;

    VkRenderPass renderPass;
    std::vector<VkFramebuffer> framebuffers;
    Attachment colorAttachment;
    Attachment depthAttachment;

    WvkSwapchain &swapchain;
    WvkDevice &device;
};

};
