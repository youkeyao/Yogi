#include "platform/opengl/opengl_buffer.h"
#include <glad/glad.h>

namespace hazel {

    //
    // Vertex buffer
    //

    OpenGLVertexBuffer::OpenGLVertexBuffer(float* vertices, uint32_t size)
    {
        glGenBuffers(1, &m_renderer_id);
        glBindBuffer(GL_ARRAY_BUFFER, m_renderer_id);
        glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
    }

    OpenGLVertexBuffer::~OpenGLVertexBuffer() { glDeleteBuffers(1, &m_renderer_id); }

    void OpenGLVertexBuffer::bind() const { glBindBuffer(GL_ARRAY_BUFFER, m_renderer_id); }

    void OpenGLVertexBuffer::unbind() const { glBindBuffer(GL_ARRAY_BUFFER, 0); }

    //
    // Index buffer
    //

    OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t* indices, uint32_t count) : m_count(count)
    {
        glGenBuffers(1, &m_renderer_id);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_renderer_id);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
    }

    OpenGLIndexBuffer::~OpenGLIndexBuffer() { glDeleteBuffers(1, &m_renderer_id); }

    void OpenGLIndexBuffer::bind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_renderer_id); }

    void OpenGLIndexBuffer::unbind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }

}