#include "platform/renderer/opengl/opengl_buffer.h"
#include <glad/glad.h>

namespace hazel {

    Ref<VertexBuffer> VertexBuffer::create(float* vertices, uint32_t size)
    {
        return CreateRef<OpenGLVertexBuffer>(vertices, size);
    }

    Ref<IndexBuffer> IndexBuffer::create(uint32_t* indices, uint32_t size)
    {
        return CreateRef<OpenGLIndexBuffer>(indices, size);
    }

    //
    // Vertex buffer
    //

    OpenGLVertexBuffer::OpenGLVertexBuffer(float* vertices, uint32_t size)
    {
        HZ_PROFILE_FUNCTION();

        glCreateBuffers(1, &m_renderer_id);
        glBindBuffer(GL_ARRAY_BUFFER, m_renderer_id);
        glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
    }

    OpenGLVertexBuffer::~OpenGLVertexBuffer()
    {
        HZ_PROFILE_FUNCTION();

        glDeleteBuffers(1, &m_renderer_id);
    }

    void OpenGLVertexBuffer::bind() const
    {
        HZ_PROFILE_FUNCTION();

        glBindBuffer(GL_ARRAY_BUFFER, m_renderer_id);
    }

    void OpenGLVertexBuffer::unbind() const
    {
        HZ_PROFILE_FUNCTION();

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    //
    // Index buffer
    //

    OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t* indices, uint32_t count) : m_count(count)
    {
        HZ_PROFILE_FUNCTION();

        glCreateBuffers(1, &m_renderer_id);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_renderer_id);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
    }

    OpenGLIndexBuffer::~OpenGLIndexBuffer()
    {
        HZ_PROFILE_FUNCTION();
        glDeleteBuffers(1, &m_renderer_id);
    }

    void OpenGLIndexBuffer::bind() const
    {
        HZ_PROFILE_FUNCTION();
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_renderer_id);
    }

    void OpenGLIndexBuffer::unbind() const
    {
        HZ_PROFILE_FUNCTION();
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

}