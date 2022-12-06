#include "platform/opengl/opengl_vertex_array.h"
#include <glad/glad.h>

namespace hazel {

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
        HZ_CORE_ASSERT(false, "unknown ShaderDataType!");
        return 0;
    }

    OpenGLVertexArray::OpenGLVertexArray()
    {
        glGenVertexArrays(1, &m_renderer_id);
        glBindVertexArray(m_renderer_id);
    }

    OpenGLVertexArray::~OpenGLVertexArray() { glDeleteVertexArrays(1, &m_renderer_id); }

    void OpenGLVertexArray::bind() const { glBindVertexArray(m_renderer_id); }

    void OpenGLVertexArray::unbind() const { glBindVertexArray(0); }

    void OpenGLVertexArray::add_vertex_buffer(const std::shared_ptr<VertexBuffer>& vertex_buffer) 
    {
        HZ_CORE_ASSERT(vertex_buffer->get_layout().get_elements().size(), "vertex buffer has no layout");

        glBindVertexArray(m_renderer_id);
        vertex_buffer->bind();

        uint32_t index = 0;
        const auto& layout = vertex_buffer->get_layout();
        for (const auto& element : layout) {
            glEnableVertexAttribArray(index);
            glVertexAttribPointer(index, element.get_component_count(),
                                ShaderDataType_to_OpenGLBaseType(element.type),
                                element.normalized ? GL_TRUE : GL_FALSE, layout.get_stride(),
                                (const void*) (uintptr_t) element.offset);
            index++;
        }

        m_vertex_buffers.push_back(vertex_buffer);
    }

    void OpenGLVertexArray::set_index_buffer(const std::shared_ptr<IndexBuffer>& index_buffer)
    {
        glBindVertexArray(m_renderer_id);
        index_buffer->bind();

        m_index_buffer = std::move(index_buffer);
    }

}