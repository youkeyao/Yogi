#include "runtime/renderer/render_command.h"
#include "runtime/core/application.h"
#include "backends/renderer/vulkan/vulkan_context.h"
#include "backends/renderer/vulkan/vulkan_buffer.h"
#include <vulkan/vulkan.h>

namespace Yogi {

    void RenderCommand::set_viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();
        context->set_viewport(x, y, width, height);
    }

    void RenderCommand::set_clear_color(const glm::vec4& color)
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();
        context->set_clear_color(color);
    }

    void RenderCommand::draw_indexed(const Ref<IndexBuffer>& index_buffer)
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();
        // index_buffer->bind();
        // vkCmdDrawIndexed(context->get_current_command_buffer(), index_buffer->get_count(), 1, 0, 0, 0);
        // if (context->get_current_pipeline()) {
        //     context->begin_command_buffer();
        //     vkCmdBindPipeline(context->get_current_command_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, context->get_current_pipeline()->get_vk_pipeline());
        //     const std::vector<VkDescriptorSet>& descriptor_sets = context->get_current_pipeline()->get_descriptor_sets();
        //     if (!descriptor_sets.empty()) vkCmdBindDescriptorSets(context->get_current_command_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, context->get_current_pipeline()->get_vk_pipeline_layout(), 0, 1, &descriptor_sets[0], 0, nullptr);
        //     vkCmdDraw(context->get_current_command_buffer(), 3, 1, 0, 0);
        //     context->end_command_buffer();
        // }
        context->set_current_index_buffer((VulkanIndexBuffer*)index_buffer.get());
    }

}