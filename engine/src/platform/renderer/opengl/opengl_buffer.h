#pragma once

#include "base/renderer/buffer.h"

namespace Yogi {

    //
    // Vertex buffer
    //

    class OpenGLVertexBuffer : public VertexBuffer
    {
    public:
        OpenGLVertexBuffer(float* vertices, uint32_t size, bool is_static);
        ~OpenGLVertexBuffer();

        void bind() const override;
        void unbind() const override;

        void set_data(const void* data, uint32_t size) override;

        const BufferLayout& get_layout() const override { return m_layout; }
        void set_layout(const BufferLayout& layout) override { m_layout = layout; }

    private:
        uint32_t m_renderer_id;
        BufferLayout m_layout;
    };

    //
    // Index buffer
    //

    class OpenGLIndexBuffer : public IndexBuffer
    {
    public:
        OpenGLIndexBuffer(uint32_t* indices, uint32_t count);
        ~OpenGLIndexBuffer();

        void bind() const override;
        void unbind() const override;

        uint32_t get_count() const override { return m_count; }

    private:
        uint32_t m_renderer_id;
        uint32_t m_count;
    };

}