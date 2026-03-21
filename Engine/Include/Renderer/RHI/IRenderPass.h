#pragma once

#include "Renderer/RHI/ITexture.h"

namespace Yogi
{

enum class LoadOp : uint8_t
{
    Load,
    Clear,
    DontCare
};

enum class StoreOp : uint8_t
{
    Store,
    DontCare
};

enum class AttachmentUsage : uint8_t
{
    ShaderRead,
    Present,
};

struct AttachmentDesc
{
    ITexture::Format Format;
    AttachmentUsage  Usage;
    LoadOp           LoadAction  = LoadOp::Clear;
    StoreOp          StoreAction = StoreOp::Store;
};

struct RenderPassDesc
{
    std::vector<AttachmentDesc> ColorAttachments;
    AttachmentDesc              DepthAttachment;
    SampleCountFlagBits         NumSamples = SampleCountFlagBits::Count1;
};

class YG_API IRenderPass
{
public:
    virtual ~IRenderPass() = default;

    const RenderPassDesc& GetDesc() const { return m_desc; }

    static Owner<IRenderPass> Create(const RenderPassDesc& desc);

protected:
    RenderPassDesc m_desc;
};

template <>
template <typename... Args>
Owner<IRenderPass> Owner<IRenderPass>::Create(Args&&... args)
{
    return Owner<IRenderPass>(IRenderPass::Create(std::forward<Args>(args)...));
}

} // namespace Yogi
