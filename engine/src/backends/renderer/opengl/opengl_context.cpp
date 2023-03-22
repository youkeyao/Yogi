#include "backends/renderer/opengl/opengl_context.h"

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
    }

    OpenGLContext::~OpenGLContext()
    {
    }

    void OpenGLContext::init()
    {
        m_window->make_gl_context();
        int status = gladLoadGLLoader((GLADloadproc)m_window->gl_get_proc_address());
        m_window->gl_set_swap_interval(1);
        YG_CORE_ASSERT(status, "Could not initialize GLad!");

        glEnable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glCreateVertexArrays(1, &m_vertex_array);

        std::string vendor = std::string(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
        std::string renderer = std::string(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
        std::string version = std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION)));
        YG_CORE_INFO("OpenGL Info:");
        YG_CORE_INFO("    Vendor:   {0}", vendor);
        YG_CORE_INFO("    Renderer: {0}", renderer);
        YG_CORE_INFO("    Version:  {0}", version);
    }

    void OpenGLContext::shutdown()
    {
        glDeleteVertexArrays(1, &m_vertex_array);
    }

    void OpenGLContext::swap_buffers()
    {
        m_window->gl_swap_buffers();
    }

    void OpenGLContext::set_vertex_layout(const Pipeline* pipeline)
    {
        m_vertex_buffer_id = 0;
        const PipelineLayout& layout = pipeline->get_vertex_layout();
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