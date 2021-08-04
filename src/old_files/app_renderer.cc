#include "app_renderer.h"

renderer::renderer(VkRenderPass renderPass, VkPipelineLayout pipelineLayout, VkExtent2D extent) {
    render_pass = renderPass;
    pipeline_layout = pipelineLayout;
    extent = extent;
}

renderer::~renderer() {

}

std::vector<VkCommandBuffer> renderer::record_command_buffers(VkFramebuffer framebuffer, int frame) {
    std::vector<VkCommandBuffer> commands;

    for (model_buffer model : models) {
        VkCommandBufferBeginInfo cmdBeginInfo{};
        cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        cmdBeginInfo.pInheritanceInfo = nullptr;

        // Create temporary command buffer
        VkCommandBuffer commandBuffer;
        VkResult result = vkBeginCommandBuffer(commandBuffer, &bufferBeginInfo);
        if (result != VK_SUCCESS) {
            logger::fatal_error("failed to begin command buffer. error code: " + std::to_string(result));
        }

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {0.f, 0.f, 0.f, 1.f};
        clearValues[1].depthStencil = {1.f, 0};

        VkRenderPassBeginInfo renderBeginInfo{};
        renderBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderBeginInfo.renderPass = render_pass;
        renderBeginInfo.framebuffer = framebuffer;
        renderBeginInfo.renderArea.offset = {0, 0};
        renderBeginInfo.renderArea.extent = extent;
        renderBeginInfo.clearValueCount = 2;
        renderBeginInfo.pClearValues = &clearValues;

        // Begin render pass
        vkCmdBeginRenderPass(commandBuffer, &renderBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Bind vertex & index buffers
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffer(commandBuffer, 0, 1, &model.get_vertex_buffer(), offsets);
        vkCmdBindIndexBuffer(commandBuffer, &model.get_index_buffer(), 0, VK_INDEX_TYPE_UINT16);

        // TODO
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets[i], 0, nullptr);

        vkCmdDrawIndexed(commandBuffer, model.num_vertices(), 1, 0, 0, 0);
    }
}
