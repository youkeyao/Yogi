#pragma once

#include "Core/EnumFlags.h"

namespace Yogi
{

enum class SampleCountFlagBits : uint8_t
{
    Count1  = 1,
    Count2  = 2,
    Count4  = 4,
    Count8  = 8,
    Count16 = 16
};

enum class TextureUsageFlags : uint8_t
{
    None            = 0,
    Sampled         = 1 << 0,
    ColorAttachment = 1 << 1,
    DepthStencil    = 1 << 2,
    Storage         = 1 << 3,
    TransferSrc     = 1 << 4,
    TransferDst     = 1 << 5,
};
YG_ENABLE_ENUM_FLAGS(TextureUsageFlags);

struct TextureDesc;

class YG_API ITexture
{
public:
    enum class Format
    {
        NONE,
        R8G8B8_UNORM,
        R8G8B8_SRGB,
        R8G8B8A8_UNORM,
        R8G8B8A8_SRGB,
        B8G8R8A8_UNORM,
        B8G8R8A8_SRGB,
        R32G32B32A32_FLOAT,
        R32G32B32_FLOAT,
        R32_FLOAT,
        D32_FLOAT,
        D24_UNORM_S8_UINT,
    };

public:
    virtual ~ITexture() = default;

    virtual uint32_t          GetWidth() const      = 0;
    virtual uint32_t          GetHeight() const     = 0;
    virtual uint32_t          GetMipLevels() const  = 0;
    virtual ITexture::Format  GetFormat() const     = 0;
    virtual TextureUsageFlags GetUsageFlags() const = 0;

    static Owner<ITexture> Create(const TextureDesc& desc);
};

struct TextureDesc
{
    uint32_t            Width;
    uint32_t            Height;
    uint32_t            MipLevels = 1;
    ITexture::Format    Format;
    TextureUsageFlags   UsageFlags = TextureUsageFlags::Sampled;
    SampleCountFlagBits NumSamples = SampleCountFlagBits::Count1;
};

template <>
template <typename... Args>
Owner<ITexture> Owner<ITexture>::Create(Args&&... args)
{
    return ITexture::Create(std::forward<Args>(args)...);
}

} // namespace Yogi
