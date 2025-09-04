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
    Color,
    DepthStencil,
    Resolve,
    Present,
    ShaderRead
};

struct AttachmentDesc
{
    ITexture::Format Format;
    AttachmentUsage  Usage;
    LoadOp           ColorLoadOp  = LoadOp::Clear;
    StoreOp          ColorStoreOp = StoreOp::Store;
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

    virtual const std::vector<AttachmentDesc>& GetColorAttachments() const = 0;
    virtual AttachmentDesc                     GetDepthAttachment() const  = 0;
    virtual SampleCountFlagBits                GetNumSamples() const       = 0;

    static Scope<IRenderPass> Create(const RenderPassDesc& desc);
};

} // namespace Yogi
