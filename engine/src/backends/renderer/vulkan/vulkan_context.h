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
        void shutdown() override;
        void swap_buffers() override;

        VkInstance get_instance() { return m_instance; }
        VkPhysicalDevice get_physical_device() { return m_physical_device; }
        VkDevice get_device() { return m_device; }
        VkQueue get_graphics_queue() { return m_graphics_queue; }
        uint32_t get_swap_chain_image_count() { return m_swap_chain_images.size(); }
        VkFormat get_swap_chain_image_format() { return m_swap_chain_image_format; }
        VkExtent2D get_swap_chain_extent() { return m_swap_chain_extent; }
        VulkanPipeline* get_current_pipeline() { return m_pipeline; }
        VulkanVertexBuffer* get_current_vertex_buffer() { return m_vertex_buffer; }
        VulkanIndexBuffer* get_current_index_buffer() { return m_index_buffer; }
        VulkanFrameBuffer* get_current_frame_buffer() { return m_current_frame_buffer ? m_current_frame_buffer : (m_image_index >= 0 ? m_swap_chain_frame_buffers[m_image_index].get() : m_swap_chain_frame_buffers[0].get()); }
        void set_has_depth_attachment(bool has_depth_attachment) { m_has_depth_attachment = has_depth_attachment; }
        void set_current_vertex_buffer(const VulkanVertexBuffer* vertex_buffer) { m_vertex_buffer = (VulkanVertexBuffer*)vertex_buffer; }
        void set_current_index_buffer(const VulkanIndexBuffer* index_buffer) { m_index_buffer = (VulkanIndexBuffer*)index_buffer; }
        void set_current_frame_buffer(const VulkanFrameBuffer* frame_buffer) { m_current_frame_buffer = (VulkanFrameBuffer*)frame_buffer; }
        void set_current_pipeline(const VulkanPipeline* pipeline)
        {
            if (pipeline != m_pipeline) {
                m_pipeline = (VulkanPipeline*)pipeline;
                create_frame_buffers();
            }
        }
        void set_window_resized() { m_window_resized = true; }
        
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

        void create_frame_buffers();

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
        void create_command_pool();
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
        VkQueue m_transfer_queue;
        VkQueue m_present_queue;

        VkSwapchainKHR m_swap_chain;
        std::vector<VkImage> m_swap_chain_images;
        VkFormat m_swap_chain_image_format;
        VkExtent2D m_swap_chain_extent;
        std::vector<Ref<VulkanTexture2D>> m_swap_chain_textures;

        bool m_has_depth_attachment = true;
        std::vector<Ref<VulkanFrameBuffer>> m_swap_chain_frame_buffers;
        std::vector<Ref<Texture2D>> m_attachments;
        VkCommandPool m_command_pool;
        std::vector<VkCommandBuffer> m_command_buffers;

        std::vector<VkSemaphore> m_image_available_semaphores;
        std::vector<VkSemaphore> m_render_finished_semaphores;
        std::vector<VkFence> m_render_command_fences;

        bool m_window_resized = false;
        uint32_t m_current_frame = 0;
        int32_t m_image_index = -1;
        VulkanPipeline* m_pipeline = nullptr;
        VulkanVertexBuffer* m_vertex_buffer = nullptr;
        VulkanIndexBuffer* m_index_buffer = nullptr;
        VulkanFrameBuffer* m_current_frame_buffer = nullptr;

        #ifdef YG_DEBUG
            Ref<Texture2D> tmp_texture;
            Ref<UniformBuffer> tmp_uniform_buffer;
        #endif
    };

}