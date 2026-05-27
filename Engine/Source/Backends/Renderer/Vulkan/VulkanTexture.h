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

    inline uint32_t         GetWidth() const override { return m_width; }
    inline uint32_t         GetHeight() const override { return m_height; }
    inline uint32_t         GetMipLevels() const override { return m_mipLevels; }
    inline ITexture::Format GetFormat() const override { return m_format; }
    inline ITexture::Usage  GetUsage() const override { return m_usage; }

    inline VkImage GetVkImage() const { return m_image; }

    VkImageLayout GetCurrentLayout() const { return m_currentLayout; }
    void          SetCurrentLayout(VkImageLayout layout) const { m_currentLayout = layout; }

private:
    void CreateVkImage(uint32_t              width,
                       uint32_t              height,
                       VkSampleCountFlagBits numSamples,
                       VkFormat              format,
                       VkImageTiling         tiling,
                       VkImageUsageFlags     usage,
                       VkMemoryPropertyFlags properties);

private:
    bool           m_ownsImage   = true;
    VkImage        m_image       = VK_NULL_HANDLE;
    VkDeviceMemory m_imageMemory = VK_NULL_HANDLE;

    mutable VkImageLayout m_currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    uint32_t            m_width;
    uint32_t            m_height;
    uint32_t            m_mipLevels = 1;
    ITexture::Format    m_format;
    ITexture::Usage     m_usage;
    SampleCountFlagBits m_numSamples;
};

} // namespace Yogi
