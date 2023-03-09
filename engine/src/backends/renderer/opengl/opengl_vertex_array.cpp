#include "backends/renderer/opengl/opengl_vertex_array.h"
#include <glad/glad.h>

namespace Yogi {

    Ref<VertexArray> VertexArray::create()
    {
        return CreateRef<OpenGLVertexArray>();
    }

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

    OpenGLVertexArray::OpenGLVertexArray()
    {
        YG_PROFILE_FUNCTION();
        
        glCreateVertexArrays(1, &m_renderer_id);
    }

    OpenGLVertexArray::~OpenGLVertexArray()
    {
        YG_PROFILE_FUNCTION();

        glDeleteVertexArrays(1, &m_renderer_id);
    }

    void OpenGLVertexArray::bind() const
    {
        YG_PROFILE_FUNCTION();

        glBindVertexArray(m_renderer_id);
    }

    void OpenGLVertexArray::unbind() const
    {
        YG_PROFILE_FUNCTION();

        glBindVertexArray(0);
    }

    void OpenGLVertexArray::add_vertex_buffer(const Ref<VertexBuffer>& vertex_buffer) 
    {
        YG_PROFILE_FUNCTION();

        YG_CORE_ASSERT(vertex_buffer->get_layout().get_elements().size(), "vertex buffer has no layout");

        glBindVertexArray(m_renderer_id);
        vertex_buffer->bind();

        const auto& layout = vertex_buffer->get_layout();
        for (const auto& element : layout) {
            switch (element.type) {
				case ShaderDataType::Float:
				case ShaderDataType::Float2:
				case ShaderDataType::Float3:
				case ShaderDataType::Float4:
				{
					glEnableVertexAttribArray(m_vertex_buffer_id);
					glVertexAttribPointer(m_vertex_buffer_id,
						element.get_component_count(),
						ShaderDataType_to_OpenGLBaseType(element.type),
						element.normalized ? GL_TRUE : GL_FALSE,
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
						element.get_component_count(),
						ShaderDataType_to_OpenGLBaseType(element.type),
						layout.get_stride(),
						(const void*) (uintptr_t) element.offset);
					m_vertex_buffer_id++;
					break;
				}
				case ShaderDataType::Mat3:
				case ShaderDataType::Mat4:
				{
					uint8_t count = element.get_component_count();
					for (uint8_t i = 0; i < count; i++) {
						glEnableVertexAttribArray(m_vertex_buffer_id);
						glVertexAttribPointer(m_vertex_buffer_id,
							count,
							ShaderDataType_to_OpenGLBaseType(element.type),
							element.normalized ? GL_TRUE : GL_FALSE,
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

        m_vertex_buffers.push_back(vertex_buffer);
    }

    void OpenGLVertexArray::set_index_buffer(const Ref<IndexBuffer>& index_buffer)
    {
        YG_PROFILE_FUNCTION();

        glBindVertexArray(m_renderer_id);
        index_buffer->bind();

        m_index_buffer = std::move(index_buffer);
    }

}