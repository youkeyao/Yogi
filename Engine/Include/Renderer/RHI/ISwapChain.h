#pragma once

#include "Core/Window.h"
#include "Renderer/RHI/IFrameBuffer.h"

namespace Yogi
{

struct SwapChainDesc
{
    uint32_t            Width;
    uint32_t            Height;
    ITexture::Format    ColorFormat;
    ITexture::Format    DepthFormat;
    SampleCountFlagBits NumSamples;
    View<Window>        Window;
};

class YG_API ISwapChain
{
public:
    virtual ~ISwapChain() = default;

    virtual uint32_t            GetWidth() const       = 0;
    virtual uint32_t            GetHeight() const      = 0;
    virtual ITexture::Format    GetColorFormat() const = 0;
    virtual ITexture::Format    GetDepthFormat() const = 0;
    virtual SampleCountFlagBits GetNumSamples() const  = 0;

    virtual View<ITexture> GetCurrentTarget() const = 0;
    virtual View<ITexture> GetCurrentDepth() const  = 0;

    virtual void AcquireNextImage()                      = 0;
    virtual void Present()                               = 0;
    virtual void Resize(uint32_t width, uint32_t height) = 0;

    static Scope<ISwapChain> Create(const SwapChainDesc& desc);
};

} // namespace Yogi
