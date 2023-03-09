#include "backends/renderer/opengl/opengl_context.h"
#include <glad/glad.h>
#if YG_WINDOW_API == 1
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
    }

    void OpenGLContext::init()
    {
        YG_PROFILE_FUNCTION();

        #if YG_WINDOW_API == 1
            glfwMakeContextCurrent((GLFWwindow*)m_window->get_native_window());
            int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
        #endif
        YG_CORE_ASSERT(status, "Could not initialize GLad!");

        std::string vendor = std::string(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
        std::string renderer = std::string(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
        std::string version = std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION)));
        YG_CORE_INFO("OpenGL Info:");
        YG_CORE_INFO("    Vendor:   {0}", vendor);
        YG_CORE_INFO("    Renderer: {0}", renderer);
        YG_CORE_INFO("    Version:  {0}", version);
    }

}