#include "runtime/renderer/render_command.h"
#include "runtime/core/application.h"
#include "backends/renderer/vulkan/vulkan_context.h"
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
        index_buffer->bind();
        vkCmdDrawIndexed(context->get_current_command_buffer(), index_buffer->get_count(), 1, 0, 0, 0);
    }

}