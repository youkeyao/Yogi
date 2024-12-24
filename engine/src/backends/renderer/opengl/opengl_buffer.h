#pragma once

#include "runtime/renderer/buffer.h"

namespace Yogi {

//
// Vertex buffer
//

class OpenGLVertexBuffer : public VertexBuffer
{
public:
    OpenGLVertexBuffer(void *vertices, uint32_t size, bool is_static);
    ~OpenGLVertexBuffer();

    void bind() const override;
    void unbind() const override;

    void set_data(const void *data, uint32_t size) override;

private:
    uint32_t m_renderer_id;
};

//
// Index buffer
//

class OpenGLIndexBuffer : public IndexBuffer
{
public:
    OpenGLIndexBuffer(uint32_t *indices, uint32_t count, bool is_static);
    ~OpenGLIndexBuffer();

    void bind() const override;
    void unbind() const override;

    void set_data(const uint32_t *indices, uint32_t size) override;

private:
    uint32_t m_renderer_id;
};

//
// Uniform buffer
//

class OpenGLUniformBuffer : public UniformBuffer
{
public:
    OpenGLUniformBuffer(uint32_t size);
    ~OpenGLUniformBuffer();

    void bind(uint32_t binding) const override;

    void set_data(const void *data, uint32_t size, uint32_t offset = 0) override;

private:
    uint32_t m_renderer_id = 0;
};

}  // namespace Yogi