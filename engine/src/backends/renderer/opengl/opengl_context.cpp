#include "backends/renderer/opengl/opengl_context.h"
#if YG_WINDOW_API == YG_WINDOW_GLFW
    #include <GLFW/glfw3.h>
#endif

namespace Yogi {

    static GLenum ShaderDataType_to_OpenGLBaseType(ShaderDataType type)
    {
        switch (type) {
            case ShaderDataType::Float:
            case ShaderDataType::Float2:
            case ShaderDataType::Float3:
            case ShaderDataType::Float4:
            case ShaderDataType::Mat3:
            case ShaderDataType::Mat4:
                return GL_FLOAT;
            case ShaderDataType::Int:
            case ShaderDataType::Int2:
            case ShaderDataType::Int3:
            case ShaderDataType::Int4:
                return GL_INT;
            case ShaderDataType::Bool:
                return GL_BOOL;
        }
        YG_CORE_ASSERT(false, "unknown ShaderDataType!");
        return 0;
    }

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

    OpenGLContext::~OpenGLContext()
    {
        glDeleteVertexArrays(1, &m_vertex_array);
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

        glCreateVertexArrays(1, &m_vertex_array);
        glBindVertexArray(m_vertex_array);
    }

    void OpenGLContext::swap_buffers()
    {        
        #if YG_WINDOW_API == YG_WINDOW_GLFW
            glfwSwapBuffers((GLFWwindow*)m_window->get_native_window());
        #endif
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void OpenGLContext::set_vertex_layout(const Shader* shader)
    {
        m_vertex_buffer_id = 0;
        const PipelineLayout& layout = shader->get_vertex_layout();
        for (const auto& element : layout) {
            switch (element.type) {
                case ShaderDataType::Float:
                case ShaderDataType::Float2:
                case ShaderDataType::Float3:
                case ShaderDataType::Float4:
                {
                    glEnableVertexAttribArray(m_vertex_buffer_id);
                    glVertexAttribPointer(m_vertex_buffer_id,
                        element.count,
                        ShaderDataType_to_OpenGLBaseType(element.type),
                        GL_FALSE,
                        layout.get_stride(),
                        (const void*) (uintptr_t) element.offset);
                    m_vertex_buffer_id++;
                    break;
                }
                case ShaderDataType::Int:
                case ShaderDataType::Int2:
                case ShaderDataType::Int3:
                case ShaderDataType::Int4:
                case ShaderDataType::Bool:
                {
                    glEnableVertexAttribArray(m_vertex_buffer_id);
                    glVertexAttribIPointer(m_vertex_buffer_id,
                        element.count,
                        ShaderDataType_to_OpenGLBaseType(element.type),
                        layout.get_stride(),
                        (const void*) (uintptr_t) element.offset);
                    m_vertex_buffer_id++;
                    break;
                }
                case ShaderDataType::Mat3:
                case ShaderDataType::Mat4:
                {
                    uint8_t count = element.count;
                    for (uint8_t i = 0; i < count; i++) {
                        glEnableVertexAttribArray(m_vertex_buffer_id);
                        glVertexAttribPointer(m_vertex_buffer_id,
                            count,
                            ShaderDataType_to_OpenGLBaseType(element.type),
                            GL_FALSE,
                            layout.get_stride(),
                            (const void*) (uintptr_t) (element.offset + sizeof(float) * count * i));
                        glVertexAttribDivisor(m_vertex_buffer_id, 1);
                        m_vertex_buffer_id++;
                    }
                    break;
                }
                default:
                    YG_CORE_ASSERT(false, "Unknown ShaderDataType!");
            }
        }
    }

}