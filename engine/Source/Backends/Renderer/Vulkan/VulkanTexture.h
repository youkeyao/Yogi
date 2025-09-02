#pragma once

#include "Renderer/RHI/ITexture.h"
#include "VulkanDeviceContext.h"

namespace Yogi
{

class VulkanTexture : public ITexture
{
public:
    VulkanTexture(const TextureDesc& desc);
    VulkanTexture(uint32_t width, uint32_t height, ITexture::Format format, ITexture::Usage usage, VkImage image);
    virtual ~VulkanTexture();

    uint32_t         GetWidth() const override { return m_width; }
    uint32_t         GetHeight() const override { return m_height; }
    ITexture::Format GetFormat() const override { return m_format; }

    inline VkImage     GetVkImage() const { return m_image; }
    inline VkImageView GetVkImageView() const { return m_imageView; }
    inline VkSampler   GetVkSampler() const { return m_sampler; }

private:
    void CreateVkImage(uint32_t              width,
                       uint32_t              height,
                       VkSampleCountFlagBits numSamples,
                       VkFormat              format,
                       VkImageTiling         tiling,
                       VkImageUsageFlags     usage,
                       VkMemoryPropertyFlags properties);
    void CreateVkImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    void CreateVkSampler(VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressMode);

private:
    VkImage        m_image;
    VkImageView    m_imageView;
    VkDeviceMemory m_imageMemory = VK_NULL_HANDLE;
    VkSampler      m_sampler     = VK_NULL_HANDLE;

    uint32_t            m_width;
    uint32_t            m_height;
    ITexture::Format    m_format;
    ITexture::Usage     m_usage;
    SampleCountFlagBits m_numSamples;
};

} // namespace Yogi
