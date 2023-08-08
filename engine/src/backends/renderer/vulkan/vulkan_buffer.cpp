#include "backends/renderer/vulkan/vulkan_buffer.h"
#include "runtime/core/application.h"
#include "backends/renderer/vulkan/vulkan_context.h"

namespace Yogi {

    Ref<VertexBuffer> VertexBuffer::create(void* vertices, uint32_t size, bool is_static)
    {
        return CreateRef<VulkanVertexBuffer>(vertices, size, is_static);
    }

    Ref<IndexBuffer> IndexBuffer::create(uint32_t* indices, uint32_t size, bool is_static)
    {
        return CreateRef<VulkanIndexBuffer>(indices, size, is_static);
    }

    Ref<UniformBuffer> UniformBuffer::create(uint32_t size)
    {
        return CreateRef<VulkanUniformBuffer>(size);
    }

    //
    // Vertex buffer
    //

    VulkanVertexBuffer::VulkanVertexBuffer(void* vertices, uint32_t size, bool is_static)
    {
        m_size = size;
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();

        context->create_buffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_buffer, m_buffer_memory);
        vkMapMemory(context->get_device(), m_buffer_memory, 0, size, 0, &m_buffer_mapped);
        if (vertices) set_data(vertices, size);
    }

    VulkanVertexBuffer::~VulkanVertexBuffer()
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();

        vkDeviceWaitIdle(context->get_device());
        vkUnmapMemory(context->get_device(), m_buffer_memory);
        vkDestroyBuffer(context->get_device(), m_buffer, nullptr);
        vkFreeMemory(context->get_device(), m_buffer_memory, nullptr);
    }

    void VulkanVertexBuffer::bind() const
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();
        context->set_current_vertex_buffer(this);
    }

    void VulkanVertexBuffer::unbind() const
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();
        context->set_current_vertex_buffer(nullptr);
    }

    void VulkanVertexBuffer::set_data(const void* vertices, uint32_t size)
    {
        memcpy(m_buffer_mapped, vertices, size);
    }

    //
    // Index buffer
    //

    VulkanIndexBuffer::VulkanIndexBuffer(uint32_t* indices, uint32_t count, bool is_static)
    {
        m_count = count;
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();
        VkDeviceSize size = count * sizeof(uint32_t);

        context->create_buffer(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_buffer, m_buffer_memory);
        vkMapMemory(context->get_device(), m_buffer_memory, 0, size, 0, &m_buffer_mapped);
        if (indices) set_data(indices, size);
    }

    VulkanIndexBuffer::~VulkanIndexBuffer()
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();

        vkDeviceWaitIdle(context->get_device());
        vkUnmapMemory(context->get_device(), m_buffer_memory);
        vkDestroyBuffer(context->get_device(), m_buffer, nullptr);
        vkFreeMemory(context->get_device(), m_buffer_memory, nullptr);
    }

    void VulkanIndexBuffer::bind() const
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();
        context->set_current_index_buffer(this);
    }

    void VulkanIndexBuffer::unbind() const
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();
        context->set_current_index_buffer(nullptr);
    }

    void VulkanIndexBuffer::set_data(const uint32_t* indices, uint32_t size)
    {
        memcpy(m_buffer_mapped, indices, size);
    }

    //
    // Uniform buffer
    //

    VulkanUniformBuffer::VulkanUniformBuffer(uint32_t size) : m_size(size)
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();
        context->create_buffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_buffer, m_buffer_memory);
        vkMapMemory(context->get_device(), m_buffer_memory, 0, size, 0, &m_buffer_mapped);
    }

    VulkanUniformBuffer::~VulkanUniformBuffer()
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();

        vkDeviceWaitIdle(context->get_device());
        vkUnmapMemory(context->get_device(), m_buffer_memory);
        vkDestroyBuffer(context->get_device(), m_buffer, nullptr);
        vkFreeMemory(context->get_device(), m_buffer_memory, nullptr);
    }

    void VulkanUniformBuffer::bind(uint32_t binding) const
    {
        VulkanContext* context = (VulkanContext*)Application::get().get_window().get_context();

        if (context->get_current_pipeline() && !context->get_current_pipeline()->get_descriptor_sets().empty()) {
            VkDescriptorBufferInfo buffer_info{};
            buffer_info.buffer = m_buffer;
            buffer_info.offset = 0;
            buffer_info.range = m_size;

            VkWriteDescriptorSet descriptor_write{};
            descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_write.dstSet = context->get_current_pipeline()->get_descriptor_sets()[0];
            descriptor_write.dstBinding = binding;
            descriptor_write.dstArrayElement = 0;
            descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptor_write.descriptorCount = 1;
            descriptor_write.pBufferInfo = &buffer_info;
            descriptor_write.pImageInfo = nullptr;
            descriptor_write.pTexelBufferView = nullptr;

            context->wait_render_command();
            vkUpdateDescriptorSets(context->get_device(), 1, &descriptor_write, 0, nullptr);
        }
    }

    void VulkanUniformBuffer::set_data(const void* data, uint32_t size, uint32_t offset)
    {
        memcpy(m_buffer_mapped, data, size);
    }

}