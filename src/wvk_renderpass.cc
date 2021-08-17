#include "wvk_renderpass.h"
#include "wvk_swapchain.h"

#include "wvk_helper.h"

namespace wvk {

WvkRenderPass::~WvkRenderPass() {
    VkDevice dev = device.getDevice();
    for (auto &image : images) {
        vkDestroyImageView(dev, image.view, nullptr);
        vkDestroyImage(dev, image.image, nullptr);
        vkFreeMemory(dev, image.memory, nullptr);
    }

    vkDestroyRenderPass(dev, renderPass, nullptr);

    for (auto framebuffer : framebuffers) {
        vkDestroyFramebuffer(dev, framebuffer, nullptr);
    }
}

std::vector<Attachment> WvkRenderPass::initRenderPass(const RenderPassInfo &passInfo) {
    auto attachments = createResources(passInfo);
    createRenderPass(passInfo);

    return attachments;
}

void WvkRenderPass::createFramebuffer(std::vector<VkImageView> attachments) {
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.flags = 0;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = swapchain.getExtent().width;
    framebufferInfo.height = swapchain.getExtent().height;
    framebufferInfo.layers = 1;

    framebuffers.emplace_back();
    VkResult result = vkCreateFramebuffer(device.getDevice(), &framebufferInfo, nullptr, &framebuffers[framebuffers.size()-1]);
    checkVulkanError(result, "failed to create frame buffer");
}

std::vector<Attachment> WvkRenderPass::createResources(const RenderPassInfo &passInfo) {
    VkExtent2D extent = swapchain.getExtent();
    std::vector<Attachment> attachments;

    // Allocate & create images on device
    for (const ImageInfo &imageInfo : passInfo.images) {
        if (!imageInfo.createImage) {
            attachments.push_back({VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE});
            continue;
        }

        VkFormat imageFormat;
        VkImageAspectFlags aspectFlags;
        VkImageUsageFlags usageFlags = imageInfo.usage;

        Attachment *attachment;

        switch (imageInfo.type) {
            case IMAGE_COLOR:
                imageFormat = swapchain.getColorFormat();
                aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
                usageFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

                attachment = &colorAttachment;
                break;
            case IMAGE_DEPTH:
                imageFormat = swapchain.getDepthFormat();
                aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
                usageFlags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                attachment = &depthAttachment;
                break;
        }

        VkImage image;
        VkDeviceMemory memory;

        device.createImage(extent.width, extent.height,
                           imageFormat, VK_IMAGE_TILING_OPTIMAL,
                           imageInfo.samples,
                           usageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                           image, memory);
        VkImageView view = device.createImageView(image, imageFormat, aspectFlags);

        images.push_back({image, memory, view});
        attachments.push_back({image, memory, view});
    }

    return attachments;
}

void WvkRenderPass::createRenderPass(const RenderPassInfo &passInfo) {
    // Create attachments & references
    std::vector<VkAttachmentDescription> attachments{passInfo.images.size()};
    std::vector<VkAttachmentReference> attachmentRefs{passInfo.images.size()};

    for (size_t i = 0; i < passInfo.images.size(); i++) {
        ImageInfo imageInfo = passInfo.images[i];

        VkFormat imageFormat;
        VkImageLayout imageLayout;
        switch (imageInfo.type) {
        case IMAGE_COLOR:
            imageFormat = swapchain.getColorFormat();
            imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            break;
        case IMAGE_DEPTH:
            imageFormat = swapchain.getDepthFormat();
            imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            break;
        }

        VkImageLayout finalLayout = imageLayout;
        if (imageInfo.finalLayout != VK_IMAGE_LAYOUT_UNDEFINED) {
            finalLayout = imageInfo.finalLayout;
        }

        VkAttachmentDescription attachment{};
        attachment.format = imageFormat;
        attachment.samples = imageInfo.samples;
        attachment.loadOp = imageInfo.loadOp;
        attachment.storeOp = imageInfo.storeOp;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = imageInfo.initialLayout;
        attachment.finalLayout = finalLayout;

        VkAttachmentReference attachmentRef{};
        attachmentRef.attachment = i;
        attachmentRef.layout = imageLayout;

        attachments[i] = attachment;
        attachmentRefs[i] = attachmentRef;
    }


    // Create subpass & subpass dependency
    SubpassInfo subpassInfo = passInfo.subpass;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    if (subpassInfo.colorIndex >= 0) {
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &attachmentRefs[subpassInfo.colorIndex];
    } else {
        subpass.colorAttachmentCount = 0;
        subpass.pColorAttachments = nullptr;
    }

    if (subpassInfo.depthIndex >= 0) {
        subpass.pDepthStencilAttachment = &attachmentRefs[subpassInfo.depthIndex];
    } else {
        subpass.pDepthStencilAttachment = nullptr;
    }

    if (subpassInfo.resolveIndex >= 0) {
        subpass.pResolveAttachments = &attachmentRefs[subpassInfo.resolveIndex];
    } else {
        subpass.pResolveAttachments = nullptr;
    }

    // TODO: Set stage and access masks based on actual dependencies

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask =  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;


    // Create render pass
    VkRenderPassCreateInfo renderInfo{};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderInfo.pAttachments = attachments.data();
    renderInfo.subpassCount = 1;
    renderInfo.pSubpasses = &subpass;
    renderInfo.dependencyCount = 1;
    renderInfo.pDependencies = &dependency;

    VkResult result = vkCreateRenderPass(device.getDevice(), &renderInfo, nullptr, &renderPass);
    checkVulkanError(result, "failed to create render pass");
}

};
