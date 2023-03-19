#include "runtime/renderer/render_command.h"
#include "runtime/core/application.h"
#include "backends/renderer/vulkan/vulkan_context.h"
#include "backends/renderer/vulkan/vulkan_buffer.h"
#include <vulkan/vulkan.h>

namespace Yogi {

    VkClearColorValue clear_color = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
    VkViewport viewport{ 0, 0, 0, 0, 0, 1};

    void RenderCommand::set_viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();
        context->recreate_swap_chain();
        viewport = { (float)x, (float)y, (float)width, (float)height, 0.0f, 1.0f };
    }

    void RenderCommand::set_clear_color(const glm::vec4& color)
    {
        clear_color = {{ color.x, color.y, color.z, color.w }};
    }

    void RenderCommand::draw_indexed(uint32_t count)
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();
        VulkanPipeline* pipeline = context->get_current_pipeline();
        if (pipeline) {
            VkCommandBuffer commandBuffer = context->get_current_command_buffer();
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            
            VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
            YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to begin recording command buffer!");

            VkExtent2D extent = context->get_swap_chain_extent();
            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = pipeline->get_vk_render_pass();
            renderPassInfo.framebuffer = context->get_current_frame_buffer();
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = extent;
            uint32_t color_attachments_size = pipeline->get_output_layout().get_elements().size();
            std::vector<VkClearValue> clear_values;
            for (int32_t i = 0; i < color_attachments_size; i ++) {
                clear_values.push_back({clear_color});
            }
            clear_values.push_back({1.0f, 0});
            renderPassInfo.clearValueCount = static_cast<uint32_t>(clear_values.size());
            renderPassInfo.pClearValues = clear_values.data();

            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            VkViewport vp{viewport.x, viewport.y, 0, 0, 0, 1};
            vp.width = viewport.width != 0 ? viewport.width : (float)extent.width;
            vp.height = viewport.height != 0 ? -viewport.height : -(float)extent.height;
            vp.y -= vp.height;
            vkCmdSetViewport(commandBuffer, 0, 1, &vp);

            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = extent;
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            const PipelineLayout& vertex_layout = pipeline->get_vertex_layout();
            if (context->get_current_vertex_buffer()) {
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->get_vk_pipeline());
                const std::vector<VkDescriptorSet>& descriptor_sets = pipeline->get_descriptor_sets();
                if (!descriptor_sets.empty()) vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->get_vk_pipeline_layout(), 0, 1, &descriptor_sets[0], 0, nullptr);
                VkBuffer vertex_buffers[] = {context->get_current_vertex_buffer()->get_vk_buffer()};
                VkDeviceSize offsets[] = {0};
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertex_buffers, offsets);
                if (context->get_current_index_buffer()) {
                    vkCmdBindIndexBuffer(commandBuffer, context->get_current_index_buffer()->get_vk_buffer(), 0, VK_INDEX_TYPE_UINT32);
                    vkCmdDrawIndexed(commandBuffer, count, 1, 0, 0, 0);
                }
                else {
                    vkCmdDraw(commandBuffer, context->get_current_vertex_buffer()->get_size() / vertex_layout.get_stride(), 1, 0, 0);
                }
            }

            vkCmdEndRenderPass(commandBuffer);

            result = vkEndCommandBuffer(commandBuffer);
            YG_CORE_ASSERT(result == VK_SUCCESS, "Failed to record command buffer!");
        }
    }

}