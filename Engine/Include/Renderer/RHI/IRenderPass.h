#pragma once

#include "Core/EnumFlags.h"
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

enum class ResourceState : uint32_t
{
    None                   = 0,
    Common                 = 1 << 0,
    VertexShaderResource   = 1 << 1,
    FragmentShaderResource = 1 << 2,
    ComputeShaderResource  = 1 << 3,
    TaskShaderResource     = 1 << 4,
    MeshShaderResource     = 1 << 5,
    UnorderedAccess        = 1 << 6,
    CopySource             = 1 << 7,
    CopyDestination        = 1 << 8,
    ColorAttachment        = 1 << 9,
    DepthRead              = 1 << 10,
    DepthWrite             = 1 << 11,
    IndirectArg            = 1 << 12,
    Present                = 1 << 13,
    VertexBuffer           = 1 << 14,
    IndexBuffer            = 1 << 15,
    UniformBuffer          = 1 << 16
};
YG_ENABLE_ENUM_FLAGS(ResourceState);

struct AttachmentDesc
{
    ITexture::Format Format;
    ResourceState    FinalState  = ResourceState::FragmentShaderResource;
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
