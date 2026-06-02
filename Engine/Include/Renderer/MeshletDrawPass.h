#pragma once

#include "Renderer/RenderPass.h"
#include "Renderer/RHI/IPipeline.h"
#include "Renderer/RHI/ITextureView.h"

namespace Yogi
{

class YG_API MeshletDrawPass : public RenderPass
{
public:
    MeshletDrawPass()           = default;
    ~MeshletDrawPass() override = default;

    void SetTargetColorFormat(ITexture::Format colorFormat) { m_colorFormat = colorFormat; }

    void SetIndirectBuffers(WRef<IBuffer> indirectCommandBuffer, WRef<IBuffer> indirectCountBuffer)
    {
        m_indirectCmd   = indirectCommandBuffer;
        m_indirectCount = indirectCountBuffer;
    }

    void Initialize() override;
    void Shutdown() override;

    void ExecuteEarly(ICommandBuffer* cmd, uint64_t sceneFrameAddr, uint32_t drawBase, uint32_t drawCount);
    void ExecuteLate(ICommandBuffer* cmd,
                     uint64_t        sceneFrameAddr,
                     uint32_t        drawBase,
                     uint32_t        drawCount,
                     uint32_t        pyramidSlot);

    const IPipeline* GetEarlyPipeline() const { return m_earlyPipeline.Get(); }
    const IPipeline* GetLatePipeline() const { return m_latePipeline.Get(); }

private:
    WRef<IBuffer> m_indirectCmd   = nullptr;
    WRef<IBuffer> m_indirectCount = nullptr;

    WRef<IPipeline> m_earlyPipeline = nullptr;
    WRef<IPipeline> m_latePipeline  = nullptr;

    ITexture::Format m_colorFormat = ITexture::Format::NONE;
};

} // namespace Yogi
