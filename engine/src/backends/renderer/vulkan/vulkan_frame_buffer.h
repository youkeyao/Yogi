#pragma once

#include "runtime/renderer/frame_buffer.h"
#include <vulkan/vulkan.h>

namespace Yogi {

    class VulkanFrameBuffer : public FrameBuffer
    {
    public:
        VulkanFrameBuffer(uint32_t width, uint32_t height, const std::vector<Ref<Texture2D>>& color_attachments);
        VulkanFrameBuffer(VkRenderPass render_pass = VK_NULL_HANDLE, bool has_depth_attachment = true);
        ~VulkanFrameBuffer();

        void bind() const override;
        void unbind() const override;

        void resize(uint32_t width, uint32_t height) override;
        uint32_t get_width() const override { return m_width; }
        uint32_t get_height() const override { return m_height; }

        void add_color_attachment(uint32_t index, const Ref<Texture2D>& attachment) override;
        void remove_color_attachment(uint32_t index) override;
        const Ref<Texture2D>& get_color_attachment(uint32_t index) const override;

        VkFramebuffer get_vk_frame_buffer() const { return m_frame_buffer; }
        void create_vk_frame_buffer(const std::vector<VkImageView>& image_views);
        void cleanup_vk_frame_buffer();
        void set_vk_extent(VkExtent2D extent) { m_width = extent.width; m_height = extent.height; }
    private:
        VkFramebuffer m_frame_buffer = VK_NULL_HANDLE;
        uint32_t m_width, m_height;

        Ref<Texture2D> m_color_attachments[4] = { nullptr, nullptr, nullptr, nullptr };
        uint32_t m_color_attachments_size = 0;

        VkImage m_depth_image = VK_NULL_HANDLE;
        VkDeviceMemory m_depth_image_memory = VK_NULL_HANDLE;
        VkImageView m_depth_image_view = VK_NULL_HANDLE;

        bool has_depth_attachment = true;
        VkRenderPass m_render_pass = VK_NULL_HANDLE;
    };

}