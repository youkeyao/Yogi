#include "backends/renderer/opengl/opengl_buffer.h"

#include <glad/glad.h>

namespace Yogi {

Ref<VertexBuffer> VertexBuffer::create(void *vertices, uint32_t size, bool is_static)
{
    return CreateRef<OpenGLVertexBuffer>(vertices, size, is_static);
}

Ref<IndexBuffer> IndexBuffer::create(uint32_t *indices, uint32_t count, bool is_static)
{
    return CreateRef<OpenGLIndexBuffer>(indices, count, is_static);
}

Ref<UniformBuffer> UniformBuffer::create(uint32_t size)
{
    return CreateRef<OpenGLUniformBuffer>(size);
}

//
// Vertex buffer
//

OpenGLVertexBuffer::OpenGLVertexBuffer(void *vertices, uint32_t size, bool is_static)
{
    m_size = size;

    glCreateBuffers(1, &m_renderer_id);
    glBindBuffer(GL_ARRAY_BUFFER, m_renderer_id);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, is_static ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW);
}

OpenGLVertexBuffer::~OpenGLVertexBuffer()
{
    glDeleteBuffers(1, &m_renderer_id);
}

void OpenGLVertexBuffer::bind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, m_renderer_id);
}

void OpenGLVertexBuffer::unbind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void OpenGLVertexBuffer::set_data(const void *data, uint32_t size)
{
    glBindBuffer(GL_ARRAY_BUFFER, m_renderer_id);
    glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
}

//
// Index buffer
//

OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t *indices, uint32_t count, bool is_static)
{
    m_count = count;

    glCreateBuffers(1, &m_renderer_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_renderer_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), indices, is_static ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW);
}

OpenGLIndexBuffer::~OpenGLIndexBuffer()
{
    glDeleteBuffers(1, &m_renderer_id);
}

void OpenGLIndexBuffer::bind() const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_renderer_id);
}

void OpenGLIndexBuffer::unbind() const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void OpenGLIndexBuffer::set_data(const uint32_t *indices, uint32_t size)
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_renderer_id);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, size, indices);
}

//
// Uniform buffer
//

OpenGLUniformBuffer::OpenGLUniformBuffer(uint32_t size)
{
    glCreateBuffers(1, &m_renderer_id);
    glNamedBufferData(m_renderer_id, size, nullptr, GL_DYNAMIC_DRAW);
}

OpenGLUniformBuffer::~OpenGLUniformBuffer()
{
    glDeleteBuffers(1, &m_renderer_id);
}

void OpenGLUniformBuffer::bind(uint32_t binding) const
{
    glBindBufferBase(GL_UNIFORM_BUFFER, binding, m_renderer_id);
}

void OpenGLUniformBuffer::set_data(const void *data, uint32_t size, uint32_t offset)
{
    glNamedBufferSubData(m_renderer_id, offset, size, data);
}

}  // namespace Yogi