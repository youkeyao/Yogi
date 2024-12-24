#pragma once

namespace Yogi {

class VertexBuffer
{
public:
    virtual ~VertexBuffer() = default;

    virtual void bind() const = 0;
    virtual void unbind() const = 0;

    virtual void set_data(const void *vertices, uint32_t size) = 0;

    uint32_t get_size() const { return m_size; }

    static Ref<VertexBuffer> create(void *vertices, uint32_t size, bool is_static = true);

protected:
    uint32_t m_size = 0;
};

class IndexBuffer
{
public:
    virtual ~IndexBuffer() = default;

    virtual void bind() const = 0;
    virtual void unbind() const = 0;

    virtual void set_data(const uint32_t *indices, uint32_t size) = 0;

    uint32_t get_count() const { return m_count; }

    static Ref<IndexBuffer> create(uint32_t *indices, uint32_t count, bool is_static = true);

protected:
    uint32_t m_count = 0;
};

class UniformBuffer
{
public:
    virtual ~UniformBuffer() = default;

    virtual void bind(uint32_t binding) const = 0;

    virtual void set_data(const void *data, uint32_t size, uint32_t offset = 0) = 0;

    static Ref<UniformBuffer> create(uint32_t size);
};

}  // namespace Yogi