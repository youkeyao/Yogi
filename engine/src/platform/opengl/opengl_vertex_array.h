#pragma once

#include "renderer/vertex_array.h"

namespace hazel {

    class OpenGLVertexArray : public VertexArray
    {
    public:
        OpenGLVertexArray();
        ~OpenGLVertexArray();

        void bind() const override;
        void unbind() const override;

        void add_vertex_buffer(const std::shared_ptr<VertexBuffer>& vertex_buffer) override;
        void set_index_buffer(const std::shared_ptr<IndexBuffer>& index_buffer) override;

        const std::vector<std::shared_ptr<VertexBuffer>>& get_vertex_buffers() const override { return m_vertex_buffers; }
        const std::shared_ptr<IndexBuffer>& get_index_buffer() const override { return m_index_buffer; }

    private:
        uint32_t m_renderer_id;
        std::vector<std::shared_ptr<VertexBuffer>> m_vertex_buffers;
        std::shared_ptr<IndexBuffer> m_index_buffer;
    };

}