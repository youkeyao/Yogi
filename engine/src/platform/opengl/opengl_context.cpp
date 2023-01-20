#include "platform/opengl/opengl_context.h"
#include <glad/glad.h>

namespace hazel {

    OpenGLContext::OpenGLContext(GLFWwindow* window) : m_window(window)
    {
        HZ_CORE_ASSERT(m_window, "window handle is null!");
    }

    void OpenGLContext::init()
    {
        HZ_PROFILE_FUNCTION();

        glfwMakeContextCurrent(m_window);
        int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
        HZ_CORE_ASSERT(status, "Could not initialize GLad!");

        std::string vendor = std::string(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
        std::string renderer = std::string(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
        std::string version = std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION)));
        HZ_CORE_INFO("OpenGL Info:");
        HZ_CORE_INFO("    Vendor:   {0}", vendor);
        HZ_CORE_INFO("    Renderer: {0}", renderer);
        HZ_CORE_INFO("    Version:  {0}", version);
    }

    void OpenGLContext::swap_buffers()
    {
        HZ_PROFILE_FUNCTION();
        
        glfwSwapBuffers(m_window);
    }

}