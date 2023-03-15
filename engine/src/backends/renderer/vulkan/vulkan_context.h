#pragma once

#include "runtime/renderer/graphics_context.h"
#include "backends/renderer/vulkan/vulkan_shader.h"
#include "backends/renderer/vulkan/vulkan_buffer.h"
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

namespace Yogi
{

    class VulkanContext : public GraphicsContext
    {
    public:
        VulkanContext(Window* window);
        ~VulkanContext();

        void init() override;
        void swap_buffers() override;

        VkDevice get_device() { return m_device; }
        VkCommandBuffer get_current_command_buffer() { return m_command_buffers[m_current_frame]; }
        VkRenderPass get_render_pass() { return m_render_pass; }
        VulkanShader* get_current_pipeline() { return m_pipeline; }
        void set_current_pipeline(const VulkanShader* pipeline) { m_pipeline = (VulkanShader*)pipeline; }
        void set_current_vertex_buffer(const VulkanVertexBuffer* vertex_buffer) { m_vertex_buffer = (VulkanVertexBuffer*)vertex_buffer; }
        void set_current_index_buffer(const VulkanIndexBuffer* index_buffer) { m_index_buffer = (VulkanIndexBuffer*)index_buffer; is_draw = true; }
        std::vector<VkDynamicState> get_dynamic_states() { return std::vector<VkDynamicState>{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR }; }
        void set_clear_color(const glm::vec4& color) { m_clear_color = { color.x, color.y, color.z, color.w }; }
        void set_viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
        {
            m_viewport = { (float)x, (float)y, (float)width, (float)height, 0.0f, 1.0f };
            is_window_resized = true;
        };

        uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties);
        void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory);
        void copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);
    private:
        void create_instance();
        void setup_debug_messenger();
        void create_surface();
        void pick_physical_device();
        void create_logical_device();
        void create_swap_chain();
        void create_image_views();
        void create_render_pass();
        void create_frame_buffers();
        void create_command_pool();
        void create_command_buffer();
        void create_sync_objects();

        void cleanup_swap_chain();
        void recreate_swap_chain();
        void record_render_command();
    private:
        Window* m_window;
        VkInstance m_instance;
        VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
        VkDevice m_device;

        VkQueue m_graphics_queue;
        VkQueue m_present_queue;

        VkSwapchainKHR m_swap_chain;
        std::vector<VkImage> m_swap_chain_images;
        VkFormat m_swap_chain_image_format;
        VkExtent2D m_swap_chain_extent;
        std::vector<VkImageView> m_swap_chain_image_views;

        VkRenderPass m_render_pass;

        std::vector<VkFramebuffer> m_swap_chain_frame_buffers;
        VkCommandPool m_command_pool;
        std::vector<VkCommandBuffer> m_command_buffers;

        std::vector<VkSemaphore> m_image_available_semaphores;
        std::vector<VkSemaphore> m_render_finished_semaphores;
        std::vector<VkFence> m_in_flight_fences;

        uint32_t m_current_frame = 0;
        uint32_t m_image_index = 0;
        VkViewport m_viewport{ 0, 0, 0, 0, 0, 1};
        VkClearValue m_clear_color = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
        VulkanShader* m_pipeline = nullptr;
        VulkanVertexBuffer* m_vertex_buffer = nullptr;
        VulkanIndexBuffer* m_index_buffer = nullptr;

        bool is_window_resized = false;
        bool is_draw = true;
    };

}