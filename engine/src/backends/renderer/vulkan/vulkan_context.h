#pragma once

#include "runtime/renderer/graphics_context.h"
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

        VkShaderModule create_shader_module(const std::vector<uint8_t>& code);
        void record_command_buffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
        void recreate_swap_chain();
    private:
        void create_instance();
        void setup_debug_messenger();
        void create_surface();
        void pick_physical_device();
        void create_logical_device();
        void create_swap_chain();
        void create_image_views();
        void create_render_pass();
        void create_graphics_pipeline();
        void create_frame_buffers();
        void create_command_pool();
        void create_command_buffer();
        void create_sync_objects();

        void cleanup_swap_chain();
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
        VkPipelineLayout m_pipeline_layout;
        VkPipeline m_graphics_pipeline;

        std::vector<VkFramebuffer> m_swap_chain_frame_buffers;
        VkCommandPool m_command_pool;
        std::vector<VkCommandBuffer> m_command_buffers;

        std::vector<VkSemaphore> m_image_available_semaphores;
        std::vector<VkSemaphore> m_render_finished_semaphores;
        std::vector<VkFence> m_in_flight_fences;

        uint32_t m_current_frame = 0;
    };

}