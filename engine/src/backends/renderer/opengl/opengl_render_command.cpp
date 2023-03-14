#include "runtime/renderer/render_command.h"
#include <glad/glad.h>

namespace Yogi {

    void RenderCommand::set_viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
        glViewport(x, y, width, height);
    }

    void RenderCommand::set_clear_color(const glm::vec4& color)
    {
        glClearColor(color.r, color.g, color.b, color.a);
    }

    void RenderCommand::draw_indexed(const Ref<VertexArray>& vertex_array, uint32_t index_count)
    {
        uint32_t count = index_count ? index_count : vertex_array->get_index_buffer()->get_count();
        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

}