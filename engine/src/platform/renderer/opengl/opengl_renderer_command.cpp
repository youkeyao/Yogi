#include "base/renderer/renderer_command.h"
#include <glad/glad.h>

namespace hazel {

    void RendererCommand::init()
    {
        HZ_PROFILE_FUNCTION();
        
        glEnable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    void RendererCommand::set_viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
        glViewport(x, y, width, height);
    }

    void RendererCommand::set_clear_color(const glm::vec4& color)
    {
        glClearColor(color.r, color.g, color.b, color.a);
    }

    void RendererCommand::clear()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void RendererCommand::draw_indexed(const Ref<VertexArray>& vertex_array)
    {
        glDrawElements(GL_TRIANGLES, vertex_array->get_index_buffer()->get_count(), GL_UNSIGNED_INT, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

}