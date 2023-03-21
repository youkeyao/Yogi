#pragma once

#include "runtime/renderer/graphics_context.h"
#include "backends/renderer/vulkan/vulkan_pipeline.h"
#include "backends/renderer/vulkan/vulkan_buffer.h"
#include "backends/renderer/vulkan/vulkan_texture.h"
#include "backends/renderer/vulkan/vulkan_frame_buffer.h"
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

        VkInstance get_instance() { return m_instance; }
        VkPhysicalDevice get_physical_device() { return m_physical_device; }
        VkDevice get_device() { return m_device; }
        VkQueue get_graphics_queue() { return m_graphics_queue; }
        uint32_t get_swap_chain_image_count() { return m_swap_chain_images.size(); }
        VkFormat get_swap_chain_image_format() { return m_swap_chain_image_format; }
        const std::vector<VkImageView>& get_swap_chain_image_views() { return m_swap_chain_image_views; }
        VkExtent2D get_swap_chain_extent() { return m_swap_chain_extent; }
        VkCommandPool get_command_pool() { return m_command_pool; }
        VulkanPipeline* get_current_pipeline() { return m_pipeline; }
        VulkanVertexBuffer* get_current_vertex_buffer() { return m_vertex_buffer; }
        VulkanIndexBuffer* get_current_index_buffer() { return m_index_buffer; }
        uint32_t get_current_frame() { return m_current_frame; }
        VkFramebuffer get_current_frame_buffer() { return m_current_frame_buffer ? m_current_frame_buffer : m_swap_chain_frame_buffers[m_image_index]->get_vk_frame_buffer(); }
        uint32_t get_current_present_image_index() { return m_image_index; }
        void set_current_vertex_buffer(const VulkanVertexBuffer* vertex_buffer) { m_vertex_buffer = (VulkanVertexBuffer*)vertex_buffer; }
        void set_current_index_buffer(const VulkanIndexBuffer* index_buffer) { m_index_buffer = (VulkanIndexBuffer*)index_buffer; }
        void set_default_frame_buffer(uint32_t index, Ref<VulkanFrameBuffer> frame_buffer) { m_swap_chain_frame_buffers[index] = frame_buffer; }
        void set_current_frame_buffer(VkFramebuffer frame_buffer) { m_current_frame_buffer = frame_buffer; }
        void set_current_pipeline(const VulkanPipeline* pipeline)
        {
            if (m_pipeline) {
                for (auto& framebuffer : m_swap_chain_frame_buffers) {
                    framebuffer->cleanup_vk_frame_buffer();
                }
            }
            m_pipeline = (VulkanPipeline*)pipeline;
            create_frame_buffers();
        }
        
        uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties);
        VkFormat find_depth_format();
        void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory);
        void create_image(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& image_memory);
        VkImageView create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags);
        VkRenderPass create_render_pass(const std::vector<VkFormat>& color_attachment_formats, VkImageLayout init_layout, VkImageLayout final_layout, bool has_depth_attachment = true);
        void copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);
        void transition_image_layout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout, VkImageAspectFlags aspect_flags);
        VkCommandBuffer begin_single_time_commands();
        void end_single_time_commands(VkCommandBuffer command_buffer);
        void recreate_swap_chain();
        VkCommandBuffer begin_render_command();
        void end_render_command();
        void wait_render_command();
        void begin_frame();

        #ifdef YG_DEBUG
            VulkanTexture2D* get_tmp_texture() { return (VulkanTexture2D*)tmp_texture.get(); }
            VulkanUniformBuffer* get_tmp_uniform_buffer() { return (VulkanUniformBuffer*)tmp_uniform_buffer.get(); }
        #endif

        std::vector<VkDynamicState> get_dynamic_states() { return std::vector<VkDynamicState>{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR }; }
    private:
        void create_instance();
        void setup_debug_messenger();
        void create_surface();
        void pick_physical_device();
        void create_logical_device();
        void create_swap_chain();
        void create_image_views();
        void create_command_pool();
        void create_frame_buffers();
        void create_command_buffer();
        void create_sync_objects();

        void cleanup_swap_chain();
        VkFormat find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
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

        std::vector<Ref<VulkanFrameBuffer>> m_swap_chain_frame_buffers;
        VkFramebuffer m_current_frame_buffer = nullptr;
        VkCommandPool m_command_pool;
        std::vector<VkCommandBuffer> m_command_buffers;

        std::vector<VkSemaphore> m_image_available_semaphores;
        std::vector<VkSemaphore> m_render_finished_semaphores;
        std::vector<VkFence> m_render_command_fences;

        uint32_t m_current_command_buffer_index = 0;
        uint32_t m_current_frame = 0;
        uint32_t m_image_index = 0;
        VulkanPipeline* m_pipeline = nullptr;
        VulkanVertexBuffer* m_vertex_buffer = nullptr;
        VulkanIndexBuffer* m_index_buffer = nullptr;

        #ifdef YG_DEBUG
            Ref<Texture2D> tmp_texture;
            Ref<UniformBuffer> tmp_uniform_buffer;
        #endif
    };

}