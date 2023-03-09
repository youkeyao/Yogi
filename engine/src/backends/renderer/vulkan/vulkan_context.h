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
    private:
        void create_instance();
        void setup_debug_messenger();
        void create_surface();
        void pick_physical_device();
        void create_logical_device();
        void create_swap_chain();
        void create_image_views();
        void create_graphics_pipeline();
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
    };

}