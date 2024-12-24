#include "backends/renderer/vulkan/vulkan_buffer.h"
#include "backends/renderer/vulkan/vulkan_context.h"
#include "runtime/core/application.h"
#include "runtime/renderer/render_command.h"

#include <vulkan/vulkan.h>

namespace Yogi {

VkClearColorValue clear_color = {
    { 0.0f, 0.0f, 0.0f, 1.0f }
};
VkViewport              viewport{ 0, 0, 0, 0, 0, 1 };
VkViewport              window_viewport{ 0, 0, 0, 0, 0, 1 };
bool                    is_clear = false;
PFN_vkCmdSetCullModeEXT load_vkCmdSetCullMode = nullptr;

void RenderCommand::set_viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    viewport = { (float)x, (float)y, (float)width, (float)height, 0, 1 };
    VulkanContext *context = (VulkanContext *)Application::get().get_window().get_context();
    if (context->is_render_to_screen()) {
        if (window_viewport.x != x || window_viewport.y != y || window_viewport.width != width ||
            window_viewport.height != height) {
            context->set_window_resized();
            window_viewport = viewport;
        }
    }
}

void RenderCommand::set_clear_color(const glm::vec4 &color)
{
    clear_color = {
        { color.x, color.y, color.z, color.w }
    };
}

void RenderCommand::clear()
{
    is_clear = true;
}

void RenderCommand::draw_indexed(uint32_t count)
{
    VulkanContext *context = (VulkanContext *)Application::get().get_window().get_context();

    VulkanPipeline *pipeline = context->get_current_pipeline();
    if (pipeline) {
        VkCommandBuffer    command_buffer = context->begin_render_command();
        VulkanFrameBuffer *frame_buffer = context->get_current_frame_buffer();

        VkExtent2D            extent = context->get_swap_chain_extent();
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        if (is_clear)
            renderPassInfo.renderPass = frame_buffer->get_vk_clear_render_pass();
        else
            renderPassInfo.renderPass = frame_buffer->get_vk_load_render_pass();
        is_clear = false;

        renderPassInfo.framebuffer = frame_buffer->get_vk_frame_buffer();
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = { frame_buffer->get_width(), frame_buffer->get_height() };
        uint32_t                  color_attachments_size = pipeline->get_output_layout().get_elements().size();
        std::vector<VkClearValue> clear_values;
        for (int32_t i = 0; i < color_attachments_size; i++) {
            clear_values.push_back({ clear_color });
            clear_values.push_back({ clear_color });
        }
        clear_values.push_back({ 1.0f, 0.0f });
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clear_values.size());
        renderPassInfo.pClearValues = clear_values.data();

        vkCmdBeginRenderPass(command_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        VkViewport vp{ viewport.x, viewport.y, 0, 0, 0, 1 };
        vp.width = viewport.width != 0 ? viewport.width : (float)extent.width;
        if (!load_vkCmdSetCullMode)
            load_vkCmdSetCullMode =
                (PFN_vkCmdSetCullModeEXT)vkGetInstanceProcAddr(context->get_instance(), "vkCmdSetCullModeEXT");
        if (context->is_render_to_screen()) {
            vp.height = viewport.height != 0 ? -viewport.height : -(float)extent.height;
            vp.y -= vp.height;
            load_vkCmdSetCullMode(command_buffer, VK_CULL_MODE_BACK_BIT);
        } else {
            vp.height = viewport.height != 0 ? viewport.height : (float)extent.height;
            load_vkCmdSetCullMode(command_buffer, VK_CULL_MODE_FRONT_BIT);
        }
        vkCmdSetViewport(command_buffer, 0, 1, &vp);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = { viewport.width != 0 ? (int)viewport.width : extent.width,
                           viewport.height != 0 ? (int)viewport.height : extent.height };
        vkCmdSetScissor(command_buffer, 0, 1, &scissor);

        const PipelineLayout &vertex_layout = pipeline->get_vertex_layout();
        if (context->get_current_vertex_buffer()) {
            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->get_vk_pipeline());
            const std::vector<VkDescriptorSet> &descriptor_sets = pipeline->get_descriptor_sets();
            if (!descriptor_sets.empty())
                vkCmdBindDescriptorSets(
                    command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->get_current_pipeline()->get_vk_pipeline_layout(),
                    0, 1, &descriptor_sets[0], 0, nullptr);
            VkBuffer     vertex_buffers[] = { context->get_current_vertex_buffer()->get_vk_buffer() };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
            if (context->get_current_index_buffer()) {
                vkCmdBindIndexBuffer(
                    command_buffer, context->get_current_index_buffer()->get_vk_buffer(), 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(command_buffer, count, 1, 0, 0, 0);
            } else {
                vkCmdDraw(
                    command_buffer, context->get_current_vertex_buffer()->get_size() / vertex_layout.get_stride(), 1, 0, 0);
            }
        }

        vkCmdEndRenderPass(command_buffer);

        context->end_render_command();
    }
}

}  // namespace Yogi