#pragma once

#include "backends/renderer/opengl/opengl_frame_buffer.h"
#include "runtime/renderer/graphics_context.h"
#include "runtime/renderer/pipeline.h"

#include <glad/glad.h>

namespace Yogi {

class OpenGLContext : public GraphicsContext
{
public:
    OpenGLContext(Window *window);
    ~OpenGLContext();

    void init() override;
    void shutdown() override;
    void swap_buffers() override;

    int get_msaa_samples() { return 4; }

    void set_vertex_layout(const Pipeline *pipeline);
    void set_frame_buffer(const OpenGLFrameBuffer *frame_buffer) { m_current_frame_buffer = (OpenGLFrameBuffer *)frame_buffer; }
    OpenGLFrameBuffer *get_frame_buffer() { return m_current_frame_buffer; }

private:
    Window  *m_window;
    uint32_t m_vertex_array;
    uint32_t m_vertex_buffer_id = 0;

    OpenGLFrameBuffer *m_current_frame_buffer = nullptr;
};

}  // namespace Yogi