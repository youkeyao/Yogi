#pragma once

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

class YG_API ITexture
{
public:
    enum class Format
    {
        R8G8B8A8_UNORM,
        R8G8B8A8_SRGB,
        B8G8R8A8_UNORM,
        B8G8R8A8_SRGB,
        R32G32B32A32_FLOAT,
        R32G32B32_FLOAT,
        R32_FLOAT,
        D32_FLOAT,
        D24_UNORM_S8_UINT,
        None,
    };

    enum class Usage
    {
        Texture2D,
        RenderTarget,
        DepthStencil,
    };

public:
    virtual ~ITexture() = default;

    virtual uint32_t         GetWidth() const  = 0;
    virtual uint32_t         GetHeight() const = 0;
    virtual ITexture::Format GetFormat() const = 0;
};

struct TextureDesc
{
    uint32_t            Width;
    uint32_t            Height;
    uint32_t            MipLevels = 1;
    ITexture::Format    Format;
    ITexture::Usage     Usage;
    SampleCountFlagBits NumSamples = SampleCountFlagBits::Count1;
};

} // namespace Yogi
