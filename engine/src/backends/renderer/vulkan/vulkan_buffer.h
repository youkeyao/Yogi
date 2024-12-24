#pragma once

#include "runtime/renderer/buffer.h"

#include <vulkan/vulkan.h>

namespace Yogi {

//
// Vertex buffer
//

class VulkanVertexBuffer : public VertexBuffer
{
public:
    VulkanVertexBuffer(void *vertices, uint32_t size, bool is_static);
    ~VulkanVertexBuffer();

    void bind() const override;
    void unbind() const override;

    void set_data(const void *vertices, uint32_t size) override;

    VkBuffer get_vk_buffer() { return m_buffer; }

private:
    VkBuffer       m_buffer;
    VkDeviceMemory m_buffer_memory;
    void          *m_buffer_mapped;
};

//
// Index buffer
//

class VulkanIndexBuffer : public IndexBuffer
{
public:
    VulkanIndexBuffer(uint32_t *indices, uint32_t count, bool is_static);
    ~VulkanIndexBuffer();

    void bind() const override;
    void unbind() const override;

    void set_data(const uint32_t *indices, uint32_t size) override;

    VkBuffer get_vk_buffer() { return m_buffer; }

private:
    VkBuffer       m_buffer;
    VkDeviceMemory m_buffer_memory;
    void          *m_buffer_mapped;
};

//
// Uniform buffer
//

class VulkanUniformBuffer : public UniformBuffer
{
public:
    VulkanUniformBuffer(uint32_t size);
    ~VulkanUniformBuffer();

    void bind(uint32_t binding) const override;

    void set_data(const void *data, uint32_t size, uint32_t offset = 0) override;

    VkBuffer get_vk_buffer() { return m_buffer; }

private:
    VkBuffer       m_buffer;
    VkDeviceMemory m_buffer_memory;
    void          *m_buffer_mapped;
    uint32_t       m_size;
};

}  // namespace Yogi