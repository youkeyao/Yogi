#pragma once

#include "Renderer/RHI/IRenderPass.h"

namespace Yogi
{

struct FrameBufferDesc
{
    uint32_t                    Width;
    uint32_t                    Height;
    View<IRenderPass>           RenderPass;
    std::vector<View<ITexture>> ColorAttachments;
    View<ITexture>              DepthAttachment;
};

class YG_API IFrameBuffer
{
public:
    virtual ~IFrameBuffer() = default;

    virtual uint32_t                           GetWidth() const            = 0;
    virtual uint32_t                           GetHeight() const           = 0;
    virtual View<IRenderPass>                  GetRenderPass() const       = 0;
    virtual const std::vector<View<ITexture>>& GetColorAttachments() const = 0;
    virtual View<ITexture>                     GetDepthAttachment() const  = 0;

    static Scope<IFrameBuffer> Create(const FrameBufferDesc& desc);
};

} // namespace Yogi