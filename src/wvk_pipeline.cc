#include "wvk_pipeline.h"

#include "resource_path.h"
#include "wvk_helper.h"

#include <fstream>

#include "wvk_model.h"

namespace wvk {

WvkPipeline::WvkPipeline(WvkDevice& device,
                         WvkSwapchain &swapchain,
                         VkRenderPass renderPass,
                         std::string vertShader,
                         std::string fragShader,
                         const DescriptorSetInfo &descriptorInfo,
                         const PipelineConfigInfo& config)
                         : device{device}, swapChain{swapchain}, renderPass{renderPass},
                           descriptorSetInfo{descriptorInfo} {
    createPipelineLayout();
    logger::debug("Created graphics pipeline layout");

    createGraphicsPipeline(vertShader, fragShader, config);
    logger::debug("Created graphics pipeline");

    createDescriptorPool();
    logger::debug("Created descriptor pool");

    createDescriptorSets();
    logger::debug("Created descriptor sets");
}

WvkPipeline::~WvkPipeline() {
    VkDevice dev = device.getDevice();

    vkDestroyShaderModule(dev, vertShaderModule, nullptr);
    if (fragShaderModule != VK_NULL_HANDLE) vkDestroyShaderModule(dev, fragShaderModule, nullptr);

    vkDestroyDescriptorSetLayout(dev, descriptorSetLayout, nullptr);
    vkDestroyPipelineLayout(dev, pipelineLayout, nullptr);
    vkDestroyPipeline(dev, graphicsPipeline, nullptr);

    vkDestroyDescriptorPool(dev, descriptorPool, nullptr);
}

void WvkPipeline::bind(VkCommandBuffer commandBuffer, int imageIndex) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[imageIndex], 0, nullptr);
}

static std::vector<char> readFile(const std::string& filename) {
    std::string path = resourcePath();
    std::ifstream file(path + filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        logger::fatal_error("failed to open file: " + filename);
    }

    size_t fileSize = file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}

VkShaderModule WvkPipeline::createShaderModule(const std::string& filename) {
    std::vector<char> code = readFile(filename);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(device.getDevice(), &createInfo, nullptr, &shaderModule);
    checkVulkanError(result, "failed to create shader module.");

    return shaderModule;
}

PipelineConfigInfo WvkPipeline::defaultPipelineConfigInfo() {
    PipelineConfigInfo configInfo{};
    configInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    VkRect2D scissor{};

    configInfo.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    configInfo.viewportInfo.viewportCount = 1;
    configInfo.viewportInfo.pViewports = &viewport;
    configInfo.viewportInfo.scissorCount = 1;
    configInfo.viewportInfo.pScissors = &scissor;

    configInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    configInfo.rasterizationInfo.depthClampEnable = VK_FALSE;
    configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
    configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
    configInfo.rasterizationInfo.lineWidth = 1.0f;
    configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    configInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;
    configInfo.rasterizationInfo.depthBiasConstantFactor = 0.0f;  // Optional
    configInfo.rasterizationInfo.depthBiasClamp = 0.0f;           // Optional
    configInfo.rasterizationInfo.depthBiasSlopeFactor = 0.0f;     // Optional

    configInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
    configInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    configInfo.multisampleInfo.minSampleShading = 1.0f;           // Optional
    configInfo.multisampleInfo.pSampleMask = nullptr;             // Optional
    configInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE;  // Optional
    configInfo.multisampleInfo.alphaToOneEnable = VK_FALSE;       // Optional

    configInfo.colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
      VK_COLOR_COMPONENT_A_BIT;
    configInfo.colorBlendAttachment.blendEnable = VK_FALSE;
    configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
    configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
    configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;              // Optional
    configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
    configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
    configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;              // Optional

    configInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
    configInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;  // Optional
    configInfo.colorBlendInfo.attachmentCount = 1;
    configInfo.colorBlendInfo.pAttachments = &configInfo.colorBlendAttachment;
    configInfo.colorBlendInfo.blendConstants[0] = 0.0f;  // Optional
    configInfo.colorBlendInfo.blendConstants[1] = 0.0f;  // Optional
    configInfo.colorBlendInfo.blendConstants[2] = 0.0f;  // Optional
    configInfo.colorBlendInfo.blendConstants[3] = 0.0f;  // Optional

    configInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
    configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
    configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    configInfo.depthStencilInfo.minDepthBounds = 0.0f;  // Optional
    configInfo.depthStencilInfo.maxDepthBounds = 1.0f;  // Optional
    configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;
    configInfo.depthStencilInfo.front = {};  // Optional
    configInfo.depthStencilInfo.back = {};   // Optional

    configInfo.dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    configInfo.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    configInfo.dynamicStateInfo.pDynamicStates = configInfo.dynamicStateEnables.data();
    configInfo.dynamicStateInfo.dynamicStateCount =
      static_cast<uint32_t>(configInfo.dynamicStateEnables.size());
    configInfo.dynamicStateInfo.flags = 0;

    return configInfo;
}


void WvkPipeline::createPipelineLayout() {
    // Set descriptor set bindings
    size_t len = descriptorSetInfo.layoutBindings.size();
    std::vector<VkDescriptorSetLayoutBinding> bindings{len};
    for (size_t i = 0; i < len; i++) {
        DescriptorLayoutInfo binding = descriptorSetInfo.layoutBindings[i];

        bindings[i].binding = i;
        bindings[i].descriptorType = binding.type;
        bindings[i].descriptorCount = binding.count;
        bindings[i].stageFlags = binding.stageFlags;
        bindings[i].pImmutableSamplers = nullptr; // TODO: look into usage of this field
    }

    // Create descriptor set layout containing all bindings
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    VkResult result = vkCreateDescriptorSetLayout(device.getDevice(), &layoutInfo, nullptr, &descriptorSetLayout);
    checkVulkanError(result, "failed to create descriptor set layout.");

    // Create pipeline layout (specifying descriptor sets & push constant ranges)
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

    result = vkCreatePipelineLayout(device.getDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout);
    checkVulkanError(result, "failed to create pipeline layout.");
}

void WvkPipeline::createGraphicsPipeline(std::string vertShader, std::string fragShader, const PipelineConfigInfo& config) {
    vertShaderModule = createShaderModule(vertShader);

    VkPipelineShaderStageCreateInfo vertStageInfo{};
    vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStageInfo.module = vertShaderModule;
    vertStageInfo.pName = "main";

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {vertStageInfo};

    if (fragShader.size() > 0) {
        fragShaderModule = createShaderModule(fragShader);

        VkPipelineShaderStageCreateInfo fragStageInfo{};
        fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragStageInfo.module = fragShaderModule;
        fragStageInfo.pName = "main";

        shaderStages.push_back(fragStageInfo);
    }

    // Vertex input info
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo inputInfo{};
    inputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    inputInfo.vertexBindingDescriptionCount = 1;
    inputInfo.pVertexBindingDescriptions = &bindingDescription;
    inputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    inputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    pipelineConfig = config;

    /* Create the graphics pipeline */
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount          = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages             = shaderStages.data();
    pipelineInfo.pVertexInputState   = &inputInfo;
    pipelineInfo.pInputAssemblyState = &pipelineConfig.inputAssemblyInfo;
    pipelineInfo.pTessellationState  = nullptr;
    pipelineInfo.pViewportState      = &pipelineConfig.viewportInfo;
    pipelineInfo.pRasterizationState = &pipelineConfig.rasterizationInfo;
    pipelineInfo.pMultisampleState   = &pipelineConfig.multisampleInfo;
    pipelineInfo.pDepthStencilState  = &pipelineConfig.depthStencilInfo;
    pipelineInfo.pColorBlendState    = &pipelineConfig.colorBlendInfo;
    pipelineInfo.pDynamicState       = &pipelineConfig.dynamicStateInfo;
    pipelineInfo.layout              = pipelineLayout;
    pipelineInfo.renderPass          = renderPass;

    VkResult result = vkCreateGraphicsPipelines(device.getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline);
    checkVulkanError(result, "failed to create pipeline.");
}

void WvkPipeline::createDescriptorPool() {
    uint32_t imageCount = swapChain.getImageCount();

    size_t len = descriptorSetInfo.layoutBindings.size();
    std::vector<VkDescriptorPoolSize> poolSizes{len};
    for (size_t i = 0; i < len; i++) {
        DescriptorLayoutInfo layout = descriptorSetInfo.layoutBindings[i];
        poolSizes[i].type = layout.type;
        poolSizes[i].descriptorCount = imageCount * layout.count;
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = imageCount;

    VkResult result = vkCreateDescriptorPool(device.getDevice(), &poolInfo, nullptr, &descriptorPool);
    checkVulkanError(result, "failed to create descriptor pool");
}

void WvkPipeline::createDescriptorSets() {
    uint32_t imageCount = swapChain.getImageCount();
    VkDevice dev = device.getDevice();

    std::vector<VkDescriptorSetLayout> layouts(imageCount, descriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = imageCount;
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(imageCount);
    VkResult result = vkAllocateDescriptorSets(dev, &allocInfo, descriptorSets.data());
    checkVulkanError(result, "failed to allocate descriptor sets");


    for (size_t imageIndex = 0; imageIndex < imageCount; imageIndex++) {
        std::vector<VkWriteDescriptorSet> descriptorWrite{descriptorSetInfo.layoutBindings.size()};

        VkDescriptorBufferInfo        bufferInfos[MAX_DESCRIPTORS][MAX_DESCRIPTOR_COUNT];
        VkDescriptorImageInfo   sampledImageInfos[MAX_DESCRIPTORS][MAX_DESCRIPTOR_COUNT];
        VkDescriptorImageInfo        samplerInfos[MAX_DESCRIPTORS][MAX_DESCRIPTOR_COUNT];
        VkDescriptorImageInfo  combinedImageInfos[MAX_DESCRIPTORS][MAX_DESCRIPTOR_COUNT];

        size_t    bufferInfoIndex = 0;
        size_t  sampledImageIndex = 0;
        size_t       samplerIndex = 0;
        size_t combinedImageIndex = 0;

        for (size_t layoutIndex = 0; layoutIndex < descriptorWrite.size(); layoutIndex++) {
            DescriptorLayoutInfo *layout = &descriptorSetInfo.layoutBindings[layoutIndex];
            VkWriteDescriptorSet *descriptor = &descriptorWrite[layoutIndex];

            descriptor->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor->dstSet = descriptorSets[imageIndex];
            descriptor->dstBinding = layoutIndex;
            descriptor->dstArrayElement = 0;
            descriptor->descriptorType = layout->type;
            descriptor->descriptorCount = layout->count;

            size_t descriptorSetImageIndex;
            if (layout->unique) {
                descriptorSetImageIndex = imageIndex;
            } else {
                descriptorSetImageIndex = 0;
            }

            switch (layout->type) {
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                {
                    size_t index = bufferInfoIndex++;
                    for (size_t j = 0; j < layout->count; j++) {
                        bufferInfos[index][j].buffer = layout->data[descriptorSetImageIndex][j].buffer;
                        bufferInfos[index][j].offset = 0;
                        bufferInfos[index][j].range = layout->data[descriptorSetImageIndex][j].size;
                    }

                    descriptor->pBufferInfo = bufferInfos[index];
                }
                break;
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                {
                    size_t index = sampledImageIndex++;
                    for (size_t j = 0; j < layout->count; j++) {
                        sampledImageInfos[index][j].sampler = nullptr;
                        sampledImageInfos[index][j].imageLayout = layout->data[descriptorSetImageIndex][j].imageLayout;
                        sampledImageInfos[index][j].imageView = layout->data[descriptorSetImageIndex][j].imageView;
                    }

                    descriptor->pImageInfo = sampledImageInfos[index];
                }
                break;
            case VK_DESCRIPTOR_TYPE_SAMPLER:
                {
                    size_t index = samplerIndex++;
                    for (size_t j = 0; j < layout->count; j++) {
                        samplerInfos[index][j].sampler = layout->data[descriptorSetImageIndex][j].sampler;
                    }

                    descriptor->pImageInfo = samplerInfos[index];
                }
                break;
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                {
                    size_t index = combinedImageIndex++;
                    for (size_t j = 0; j < layout->count; j++) {
                        combinedImageInfos[index][j].imageView = layout->data[descriptorSetImageIndex][j].imageView;
                        combinedImageInfos[index][j].imageLayout = layout->data[descriptorSetImageIndex][j].imageLayout;
                        combinedImageInfos[index][j].sampler = layout->data[descriptorSetImageIndex][j].sampler;
                    }

                    descriptor->pImageInfo = combinedImageInfos[index];
                }
                break;
            default:
                logger::fatal_error("Unknown VkDescriptorType when creating descriptor set. " + std::to_string(layout->type));
                break;
            }
        }

        vkUpdateDescriptorSets(dev, static_cast<uint32_t>(descriptorWrite.size()), descriptorWrite.data(), 0, nullptr);
    }
}

}
