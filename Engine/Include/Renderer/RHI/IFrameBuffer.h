#pragma once

#include "Renderer/RHI/IRenderPass.h"

namespace Yogi
{

struct FrameBufferDesc
{
    uint32_t                     Width;
    uint32_t                     Height;
    const IRenderPass*           RenderPass = nullptr;
    std::vector<const ITexture*> ColorAttachments;
    const ITexture*              DepthAttachment = nullptr;
};

class YG_API IFrameBuffer
{
public:
    virtual ~IFrameBuffer() = default;

    virtual uint32_t GetWidth() const  = 0;
    virtual uint32_t GetHeight() const = 0;

    static Owner<IFrameBuffer> Create(const FrameBufferDesc& desc);
};

template <>
template <typename... Args>
Owner<IFrameBuffer> Owner<IFrameBuffer>::Create(Args&&... args)
{
    return IFrameBuffer::Create(std::forward<Args>(args)...);
}

} // namespace Yogi