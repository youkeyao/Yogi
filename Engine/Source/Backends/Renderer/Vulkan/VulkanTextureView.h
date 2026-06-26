#pragma once

#include "Renderer/RHI/ITextureView.h"
#include "VulkanDeviceContext.h"

namespace Yogi
{

class VulkanTextureView : public ITextureView
{
public:
    VulkanTextureView(const WRef<ITexture>& texture, const TextureViewDesc& desc);
    virtual ~VulkanTextureView();

    inline const ITexture*  GetTexture() const override { return m_texture.Get(); }
    inline uint32_t         GetBaseMipLevel() const override { return m_baseMip; }
    inline uint32_t         GetMipLevelCount() const override { return m_mipCount; }
    inline uint32_t         GetBaseArrayLayer() const override { return m_baseLayer; }
    inline uint32_t         GetArrayLayerCount() const override { return m_layerCount; }
    inline ITexture::Format GetFormat() const override { return m_format; }

    void SetData(void* data, uint32_t size) override;

    inline VkImageView        GetVkImageView() const { return m_imageView; }
    inline VkImageAspectFlags GetVkViewAspectMask() const { return m_viewAspectMask; }
    inline VkImageAspectFlags GetVkBarrierAspectMask() const { return m_barrierAspectMask; }
    VkImageSubresourceRange   GetVkSubresourceRange() const;

private:
    WRef<ITexture>     m_texture;
    VkImageView        m_imageView         = VK_NULL_HANDLE;
    VkImageAspectFlags m_viewAspectMask    = 0; // For ImageView creation (can only be DEPTH or STENCIL, not both)
    VkImageAspectFlags m_barrierAspectMask = 0; // For image memory barriers (can be both DEPTH and STENCIL)
    uint32_t           m_baseMip           = 0;
    uint32_t           m_mipCount          = 1;
    uint32_t           m_baseLayer         = 0;
    uint32_t           m_layerCount        = 1;
    ITexture::Format   m_format            = ITexture::Format::NONE;
};

} // namespace Yogi
