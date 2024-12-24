#include "backends/renderer/opengl/opengl_context.h"
#include "runtime/core/application.h"
#include "runtime/renderer/render_command.h"

#include <glad/glad.h>

namespace Yogi {

void RenderCommand::set_viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    glViewport(x, y, width, height);
}

void RenderCommand::set_clear_color(const glm::vec4 &color)
{
    glClearColor(color.r, color.g, color.b, color.a);
}

void RenderCommand::clear()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void RenderCommand::draw_indexed(uint32_t count)
{
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);

    OpenGLContext     *context = (OpenGLContext *)Application::get().get_window().get_context();
    OpenGLFrameBuffer *frame_buffer = context->get_frame_buffer();
    if (frame_buffer) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, frame_buffer->get_msaa_renderer_id());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frame_buffer->get_renderer_id());
        glBlitFramebuffer(
            0, 0, frame_buffer->get_width(), frame_buffer->get_height(), 0, 0, frame_buffer->get_width(),
            frame_buffer->get_height(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer->get_msaa_renderer_id());
    }
}

}  // namespace Yogi