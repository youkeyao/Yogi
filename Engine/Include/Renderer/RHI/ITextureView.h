#pragma once

#include "Renderer/RHI/ITexture.h"
#include "Core/WRef.h"

namespace Yogi
{

struct TextureViewDesc
{
    uint32_t         BaseMipLevel    = 0;
    uint32_t         MipLevelCount   = 0; // 0 = "remaining mips from BaseMipLevel"
    uint32_t         BaseArrayLayer  = 0;
    uint32_t         ArrayLayerCount = 1;
    ITexture::Format Format          = ITexture::Format::NONE; // NONE = inherit texture format
};

class YG_API ITextureView
{
public:
    virtual ~ITextureView() = default;

    // Returns nullptr if the source texture has been destroyed.
    virtual const ITexture*  GetTexture() const         = 0;
    virtual uint32_t         GetBaseMipLevel() const    = 0;
    virtual uint32_t         GetMipLevelCount() const   = 0;
    virtual uint32_t         GetBaseArrayLayer() const  = 0;
    virtual uint32_t         GetArrayLayerCount() const = 0;
    virtual ITexture::Format GetFormat() const          = 0;

    virtual void SetData(void* data, uint32_t size) = 0;

    static Owner<ITextureView> Create(const WRef<ITexture>& texture, const TextureViewDesc& desc = {});
};

template <>
template <typename... Args>
inline Owner<ITextureView> Owner<ITextureView>::Create(Args&&... args)
{
    return ITextureView::Create(std::forward<Args>(args)...);
}

} // namespace Yogi
