#pragma once

#include "renderer/buffer.h"

namespace hazel {

    //
    // Vertex buffer
    //

    class OpenGLVertexBuffer : public VertexBuffer
    {
    public:
        OpenGLVertexBuffer(float* vertices, uint32_t size);
        ~OpenGLVertexBuffer();

        void bind() const override;
        void unbind() const override;

    private:
        uint32_t m_renderer_id;
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