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

    void SetData(void* data, uint32_t size) override;

    inline VkImage     GetVkImage() const { return m_image; }
    inline VkImageView GetVkImageView(uint32_t mipLevel = 0) const { return m_imageViews[mipLevel]; }
    inline VkSampler   GetVkSampler() const { return m_sampler; }

private:
    void CreateVkImage(uint32_t              width,
                       uint32_t              height,
                       VkSampleCountFlagBits numSamples,
                       VkFormat              format,
                       VkImageTiling         tiling,
                       VkImageUsageFlags     usage,
                       VkMemoryPropertyFlags properties);

    VkImageView CreateVkImageView(VkImage            image,
                                  VkFormat           format,
                                  VkImageAspectFlags aspectFlags,
                                  uint32_t           baseMipLevel,
                                  uint32_t           levelCount);

    void CreateVkSampler(VkFilter                       magFilter,
                         VkFilter                       minFilter,
                         VkSamplerAddressMode           addressMode,
                         ITexture::SamplerReductionMode reduction);

private:
    VkImage                  m_image;
    std::vector<VkImageView> m_imageViews;
    VkDeviceMemory           m_imageMemory = VK_NULL_HANDLE;
    VkSampler                m_sampler     = VK_NULL_HANDLE;

    uint32_t            m_width;
    uint32_t            m_height;
    uint32_t            m_mipLevels = 1;
    ITexture::Format    m_format;
    ITexture::Usage     m_usage;
    SampleCountFlagBits m_numSamples;
};

} // namespace Yogi
