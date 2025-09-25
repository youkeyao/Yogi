#pragma once

#include "Renderer/RHI/IRenderPass.h"

namespace Yogi
{

struct FrameBufferDesc
{
    uint32_t                   Width;
    uint32_t                   Height;
    Ref<IRenderPass>           RenderPass;
    std::vector<Ref<ITexture>> ColorAttachments;
    Ref<ITexture>              DepthAttachment;
};

class YG_API IFrameBuffer
{
public:
    virtual ~IFrameBuffer() = default;

    virtual uint32_t                          GetWidth() const            = 0;
    virtual uint32_t                          GetHeight() const           = 0;
    virtual Ref<IRenderPass>                  GetRenderPass() const       = 0;
    virtual const std::vector<Ref<ITexture>>& GetColorAttachments() const = 0;
    virtual Ref<ITexture>                     GetDepthAttachment() const  = 0;

    static Handle<IFrameBuffer> Create(const FrameBufferDesc& desc);
};

template <>
template <typename... Args>
Handle<IFrameBuffer> Handle<IFrameBuffer>::Create(Args&&... args)
{
    return IFrameBuffer::Create(std::forward<Args>(args)...);
}

} // namespace Yogi