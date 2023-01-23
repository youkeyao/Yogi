#pragma once

#include "base/renderer/graphics_context.h"
#include <GLFW/glfw3.h>

namespace hazel
{

    class OpenGLContext : public GraphicsContext
    {
    public:
        OpenGLContext(GLFWwindow* window);
        ~OpenGLContext() {}

        void init() override;
        void swap_buffers() override;

    private:
        GLFWwindow* m_window;
    };

}