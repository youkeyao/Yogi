#pragma once

#include "runtime/renderer/graphics_context.h"

namespace Yogi
{

    class OpenGLContext : public GraphicsContext
    {
    public:
        OpenGLContext(Window* window);
        ~OpenGLContext() {}

        void init() override;
        void swap_buffers() override;
    private:
        Window* m_window;
    };

}