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
        VulkanVertexBuffer(void* vertices, uint32_t size, bool is_static);
        ~VulkanVertexBuffer();

        void bind() const override;
        void unbind() const override;

        void set_data(const void* vertices, uint32_t size) override;
    private:
        VkBuffer m_buffer;
        VkDeviceMemory m_buffer_memory;
    };

    //
    // Index buffer
    //

    class VulkanIndexBuffer : public IndexBuffer
    {
    public:
        VulkanIndexBuffer(uint32_t* indices, uint32_t count);
        ~VulkanIndexBuffer();

        void bind() const override;
        void unbind() const override;
    private:
        VkBuffer m_buffer;
        VkDeviceMemory m_buffer_memory;
    };

    //
    // Uniform buffer
    //

    class VulkanUniformBuffer : public UniformBuffer
    {
    public:
        VulkanUniformBuffer(uint32_t size, uint32_t binding);
        ~VulkanUniformBuffer();

        void bind(uint32_t binding) const override;

        void set_data(const void* data, uint32_t size, uint32_t offset = 0) override;
    private:
        VkBuffer m_buffer;
        VkDeviceMemory m_buffer_memory;
        void* m_buffer_mapped;
        uint32_t m_size;
    };

}