#pragma once

#include "runtime/renderer/frame_buffer.h"
#include <vulkan/vulkan.h>

namespace Yogi {

    class VulkanFrameBuffer : public FrameBuffer
    {
    public:
        VulkanFrameBuffer(uint32_t width, uint32_t height, const std::vector<Ref<RenderTexture>>& color_attachments, bool has_depth_attachment = true, VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        ~VulkanFrameBuffer();

        void bind() const override;
        void unbind() const override;

        void resize(uint32_t width, uint32_t height) override;
        uint32_t get_width() const override { return m_width; }
        uint32_t get_height() const override { return m_height; }

        const Ref<RenderTexture>& get_color_attachment(uint32_t index) const override { return m_color_attachments[index]; }

        VkFramebuffer get_vk_frame_buffer() const { return m_frame_buffer; }
        VkRenderPass get_vk_clear_render_pass() const { return m_clear_render_pass; }
        VkRenderPass get_vk_load_render_pass() const { return m_load_render_pass; }
    private:
        void create_vk_frame_buffer(const std::vector<VkImageView>& image_views);
        void cleanup_vk_frame_buffer();
    private:
        VkFramebuffer m_frame_buffer = VK_NULL_HANDLE;
        uint32_t m_width, m_height;

        std::vector<Ref<RenderTexture>> m_color_attachments;
        std::vector<VkImage> m_msaa_images;
        std::vector<VkDeviceMemory> m_msaa_image_memories;
        std::vector<VkImageView> m_msaa_image_views;

        VkImage m_depth_image = VK_NULL_HANDLE;
        VkDeviceMemory m_depth_image_memory = VK_NULL_HANDLE;
        VkImageView m_depth_image_view = VK_NULL_HANDLE;

        bool m_has_depth_attachment = true;
        VkRenderPass m_clear_render_pass = VK_NULL_HANDLE;
        VkRenderPass m_load_render_pass = VK_NULL_HANDLE;
    };

}