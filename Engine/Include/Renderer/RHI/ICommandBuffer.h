#pragma once

#include "Renderer/RHI/IFrameBuffer.h"
#include "Renderer/RHI/IPipeline.h"
#include "Renderer/RHI/IBuffer.h"
#include "Renderer/RHI/ITexture.h"
#include "Renderer/RHI/IShaderResourceBinding.h"

namespace Yogi
{

enum class CommandBufferUsage : uint8_t
{
    OneTimeSubmit,
    Persistent
};

enum class SubmitQueue : uint8_t
{
    Graphics,
    Compute,
    Transfer
};

enum class PipelineStage : uint8_t
{
    None            = 0,
    DrawIndirect    = 1 << 0,
    VertexShader    = 1 << 1,
    FragmentShader  = 1 << 2,
    ComputeShader   = 1 << 3,
    TaskShader      = 1 << 4,
    MeshShader      = 1 << 5,
    Transfer        = 1 << 6,
    ColorAttachment = 1 << 7
};

YG_ENABLE_ENUM_FLAGS(PipelineStage);

enum class BarrierAccess : uint8_t
{
    None                 = 0,
    ShaderRead           = 1 << 0,
    ShaderWrite          = 1 << 1,
    IndirectCommandRead  = 1 << 2,
    TransferRead         = 1 << 3,
    TransferWrite        = 1 << 4,
    ColorAttachmentRead  = 1 << 5,
    ColorAttachmentWrite = 1 << 6
};

YG_ENABLE_ENUM_FLAGS(BarrierAccess);

union ClearValue
{
    float Color[4];
    struct
    {
        float    Depth;
        uint32_t Stencil;
    } DepthStencil;
};

struct Viewport
{
    float X, Y;
    float Width, Height;
};

struct Scissor
{
    int32_t  X, Y;
    uint32_t Width, Height;
};

struct CommandBufferDesc
{
    CommandBufferUsage Usage;
    SubmitQueue        Queue;
};

class YG_API ICommandBuffer
{
public:
    virtual ~ICommandBuffer() = default;

    virtual void Begin()  = 0;
    virtual void End()    = 0;
    virtual void Submit() = 0;
    virtual void Wait()   = 0;

    virtual void BeginRenderPass(const Ref<IFrameBuffer>&       frameBuffer,
                                 const std::vector<ClearValue>& colorClearValues,
                                 const ClearValue&              depthClearValue) = 0;
    virtual void EndRenderPass()                                    = 0;

    virtual void SetPipeline(const Ref<IPipeline>& pipeline)                          = 0;
    virtual void SetVertexBuffer(const Ref<IBuffer>& buffer, uint32_t offset = 0)     = 0;
    virtual void SetIndexBuffer(const Ref<IBuffer>& buffer, uint32_t offset = 0)      = 0;
    virtual void SetViewport(const Viewport& viewport)                                = 0;
    virtual void SetScissor(const Scissor& scissor)                                   = 0;
    virtual void SetShaderResourceBinding(const Ref<IShaderResourceBinding>& binding) = 0;
    virtual void SetPushConstants(const Ref<IShaderResourceBinding>& binding,
                                  ShaderStage                        stage,
                                  uint32_t                           offset,
                                  uint32_t                           size,
                                  const void*                        data)                                   = 0;

    virtual void Draw(uint32_t vertexCount,
                      uint32_t instanceCount = 1,
                      uint32_t firstVertex   = 0,
                      uint32_t firstInstance = 0) = 0;

    virtual void DrawIndexed(uint32_t indexCount,
                             uint32_t instanceCount = 1,
                             uint32_t firstIndex    = 0,
                             int32_t  vertexOffset  = 0,
                             uint32_t firstInstance = 0) = 0;

    virtual void DrawMeshTasks(uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) = 0;

    virtual void DrawMeshTasksIndirect(const Ref<IBuffer>& indirectBuffer,
                                       uint32_t            offset,
                                       uint32_t            drawCount,
                                       uint32_t            stride) = 0;

    virtual void DrawMeshTasksIndirectCount(const Ref<IBuffer>& indirectBuffer,
                                            uint32_t            indirectOffset,
                                            const Ref<IBuffer>& countBuffer,
                                            uint32_t            countOffset,
                                            uint32_t            maxDrawCount,
                                            uint32_t            stride) = 0;

    virtual void Dispatch(uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) = 0;
    virtual void Barrier(PipelineStage sourceStage,
                         PipelineStage destinationStage,
                         BarrierAccess sourceAccess,
                         BarrierAccess destinationAccess)                                           = 0;

    virtual void Blit(const Ref<ITexture>& src, const Ref<ITexture>& dst) = 0;

    static Owner<ICommandBuffer> Create(const CommandBufferDesc& desc);
};

template <>
template <typename... Args>
inline Owner<ICommandBuffer> Owner<ICommandBuffer>::Create(Args&&... args)
{
    return ICommandBuffer::Create(std::forward<Args>(args)...);
}

} // namespace Yogi