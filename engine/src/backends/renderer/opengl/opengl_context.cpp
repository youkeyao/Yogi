#include "backends/renderer/opengl/opengl_context.h"
#include <glad/glad.h>
#if YG_WINDOW_API == YG_WINDOW_GLFW
    #include <GLFW/glfw3.h>
#endif

namespace Yogi {

    Scope<GraphicsContext> GraphicsContext::create(Window* window)
    {
        return CreateScope<OpenGLContext>(window);
    }

    OpenGLContext::OpenGLContext(Window* window) : m_window(window)
    {
        init();

        std::string vendor = std::string(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
        std::string renderer = std::string(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
        std::string version = std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION)));
        YG_CORE_INFO("OpenGL Info:");
        YG_CORE_INFO("    Vendor:   {0}", vendor);
        YG_CORE_INFO("    Renderer: {0}", renderer);
        YG_CORE_INFO("    Version:  {0}", version);
    }

    void OpenGLContext::init()
    {
        #if YG_WINDOW_API == YG_WINDOW_GLFW
            glfwMakeContextCurrent((GLFWwindow*)m_window->get_native_window());
            int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
            glfwSwapInterval(1);
        #endif
        YG_CORE_ASSERT(status, "Could not initialize GLad!");

        glEnable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    void OpenGLContext::swap_buffers()
    {        
        #if YG_WINDOW_API == YG_WINDOW_GLFW
            glfwSwapBuffers((GLFWwindow*)m_window->get_native_window());
        #endif
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

}