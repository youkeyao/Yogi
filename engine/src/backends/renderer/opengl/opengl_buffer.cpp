#include "backends/renderer/opengl/opengl_buffer.h"
#include <glad/glad.h>

namespace Yogi {

    Ref<VertexBuffer> VertexBuffer::create(float* vertices, uint32_t size, bool is_static)
    {
        return CreateRef<OpenGLVertexBuffer>(vertices, size, is_static);
    }

    Ref<IndexBuffer> IndexBuffer::create(uint32_t* indices, uint32_t size)
    {
        return CreateRef<OpenGLIndexBuffer>(indices, size);
    }

    Ref<UniformBuffer> UniformBuffer::create(uint32_t size, uint32_t binding)
    {
        return CreateRef<OpenGLUniformBuffer>(size, binding);
    }

    //
    // Vertex buffer
    //

    OpenGLVertexBuffer::OpenGLVertexBuffer(float* vertices, uint32_t size, bool is_static)
    {
        YG_PROFILE_FUNCTION();

        glCreateBuffers(1, &m_renderer_id);
        glBindBuffer(GL_ARRAY_BUFFER, m_renderer_id);
        glBufferData(GL_ARRAY_BUFFER, size, vertices, is_static ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW);
    }

    OpenGLVertexBuffer::~OpenGLVertexBuffer()
    {
        YG_PROFILE_FUNCTION();

        glDeleteBuffers(1, &m_renderer_id);
    }

    void OpenGLVertexBuffer::bind() const
    {
        YG_PROFILE_FUNCTION();

        glBindBuffer(GL_ARRAY_BUFFER, m_renderer_id);
    }

    void OpenGLVertexBuffer::unbind() const
    {
        YG_PROFILE_FUNCTION();

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void OpenGLVertexBuffer::set_data(const void* data, uint32_t size)
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_renderer_id);
        glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
    }

    //
    // Index buffer
    //

    OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t* indices, uint32_t count) : m_count(count)
    {
        YG_PROFILE_FUNCTION();

        glCreateBuffers(1, &m_renderer_id);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_renderer_id);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
    }

    OpenGLIndexBuffer::~OpenGLIndexBuffer()
    {
        YG_PROFILE_FUNCTION();
        glDeleteBuffers(1, &m_renderer_id);
    }

    void OpenGLIndexBuffer::bind() const
    {
        YG_PROFILE_FUNCTION();
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_renderer_id);
    }

    void OpenGLIndexBuffer::unbind() const
    {
        YG_PROFILE_FUNCTION();
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    //
    // Uniform buffer
    //

    OpenGLUniformBuffer::OpenGLUniformBuffer(uint32_t size, uint32_t binding)
	{
		glCreateBuffers(1, &m_renderer_id);
		glNamedBufferData(m_renderer_id, size, nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, binding, m_renderer_id);
	}

	OpenGLUniformBuffer::~OpenGLUniformBuffer()
	{
		glDeleteBuffers(1, &m_renderer_id);
	}


	void OpenGLUniformBuffer::set_data(const void* data, uint32_t size, uint32_t offset)
	{
		glNamedBufferSubData(m_renderer_id, offset, size, data);
	}

}