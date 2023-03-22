#pragma once

#include "runtime/renderer/graphics_context.h"
#include "runtime/renderer/pipeline.h"
#include <glad/glad.h>

namespace Yogi
{

    class OpenGLContext : public GraphicsContext
    {
    public:
        OpenGLContext(Window* window);
        ~OpenGLContext();

        void init() override;
        void shutdown() override;
        void swap_buffers() override;

        void set_vertex_layout(const Pipeline* pipeline);
    private:
        Window* m_window;
        uint32_t m_vertex_array;
        uint32_t m_vertex_buffer_id = 0;
    };

}