#pragma once

#include "runtime/renderer/texture.h"
#include <vulkan/vulkan.h>

namespace Yogi {

    class VulkanTexture2D : public Texture2D
    {
    public:
        VulkanTexture2D(const std::string& name, const std::string& path);
        ~VulkanTexture2D();

        uint32_t get_width() const override { return m_width; }
        uint32_t get_height() const override{ return m_height; }
        void read_pixel(int32_t x, int32_t y, void* data) const override;

        void set_data(void* data, size_t size) override;
        void bind(uint32_t binding = 0, uint32_t slot = 0) const override;
    private:
        uint32_t m_width, m_height;
        VkImage m_image;
        VkDeviceMemory m_image_memory = VK_NULL_HANDLE;
        VkFormat m_internal_format;
        VkImageView m_image_view;
        VkSampler m_sampler = VK_NULL_HANDLE;
    };

    class VulkanRenderTexture : public RenderTexture
    {
    public:
        VulkanRenderTexture(const std::string& name, uint32_t width, uint32_t height, TextureFormat format = TextureFormat::RGBA8);
        VulkanRenderTexture(uint32_t width, uint32_t height, VkImage vk_image, VkFormat vk_format);
        ~VulkanRenderTexture();

        uint32_t get_width() const override { return m_width; }
        uint32_t get_height() const override{ return m_height; }
        void read_pixel(int32_t x, int32_t y, void* data) const override;

        void set_data(void* data, size_t size) override;
        void bind(uint32_t binding = 0, uint32_t slot = 0) const override;
        void resize(uint32_t width, uint32_t height) override;

        VkImage get_vk_image() const { return m_image; }
        VkImageView get_vk_image_view() const { return m_image_view; }
        VkFormat get_vk_format() const { return m_internal_format; }
        VkSampler get_vk_sampler() const { return m_sampler; }
    private:
        uint32_t m_width, m_height;
        VkImage m_image;
        VkDeviceMemory m_image_memory = VK_NULL_HANDLE;
        VkFormat m_internal_format;
        VkImageView m_image_view;
        VkSampler m_sampler = VK_NULL_HANDLE;
    };

}