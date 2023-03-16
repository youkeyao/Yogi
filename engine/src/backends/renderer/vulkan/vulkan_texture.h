#pragma once

#include "runtime/renderer/texture.h"
#include <vulkan/vulkan.h>

namespace Yogi {

    class VulkanTexture2D : public Texture2D
    {
    public:
        VulkanTexture2D(uint32_t width, uint32_t height, TextureFormat format = TextureFormat::RGBA8);
        VulkanTexture2D(const std::string& path);
        ~VulkanTexture2D();

        uint32_t get_width() const override { return m_width; }
        uint32_t get_height() const override{ return m_height; }
        uint32_t get_renderer_id() const override { return m_renderer_id; }
        void read_pixel(int32_t x, int32_t y, void* data) const override;

        void set_data(void* data, size_t size) override;

        void bind(uint32_t slot = 0) const override;
    private:
        void init_texture();
    private:
        uint32_t m_width, m_height;
        uint32_t m_renderer_id;
        VkImage m_image;
        VkDeviceMemory m_image_memory;
        VkFormat m_internal_format;
        VkImageView m_image_view;
        VkSampler m_sampler;
    };

}