#pragma once

#include "Renderer/RenderPass.h"
#include "Renderer/RHI/IPipeline.h"
#include "Renderer/RHI/ITextureView.h"

namespace Yogi
{

// Two-phase mesh-task drawing. Owns:
//   - EARLY / LATE graphics pipelines (task + mesh + frag), compiled twice
//     off Test.task via the "::LATE=1" key suffix; LATE adds Hi-Z occlusion
//     + meshlet-vis updates.
//
// Does NOT own the indirect command / count buffers it draws from -- the
// caller wires those in via SetIndirectBuffers before Initialize. This pass
// is therefore decoupled from any specific cull-pass implementation: anything
// that produces a (commandBuffer, countBuffer) pair in the right format can
// drive these draws.
//
// Indirect format expected (matches what ObjectCullPass writes today):
//   - command buffer: tightly packed groupCountX/Y/Z triples (12 bytes each)
//   - count buffer: a single uint at offset 0 = number of triples written
class YG_API MeshletDrawPass : public RenderPass
{
public:
    MeshletDrawPass()           = default;
    ~MeshletDrawPass() override = default;

    // Configure before Initialize.
    void SetTargetColorFormat(ITexture::Format colorFormat) { m_colorFormat = colorFormat; }

    // Caller must keep both buffers alive for the pass's lifetime. The
    // pointers are stored raw (no ref bump) -- in this engine ForwardRender-
    // System owns both this pass and the cull pass producing the indirect
    // stream, and member-destruction order guarantees the cull pass outlives
    // this one.
    void SetIndirectBuffers(IBuffer* indirectCommandBuffer, IBuffer* indirectCountBuffer)
    {
        m_indirectCmd   = indirectCommandBuffer;
        m_indirectCount = indirectCountBuffer;
    }

    void Initialize() override;
    void Shutdown() override;

    // EARLY mesh-task draw.
    void ExecuteEarly(ICommandBuffer* cmd,
                      uint64_t        sceneFrameAddr,
                      uint32_t        drawBase,
                      uint32_t        drawCount);

    // LATE mesh-task draw.
    void ExecuteLate(ICommandBuffer* cmd,
                     uint64_t        sceneFrameAddr,
                     uint32_t        drawBase,
                     uint32_t        drawCount,
                     uint32_t        pyramidSlot);

    IPipeline* GetEarlyPipeline() const { return m_earlyPipeline.Get(); }
    IPipeline* GetLatePipeline() const { return m_latePipeline.Get(); }

private:
    // Externally-owned indirect input. Set by SetIndirectBuffers before
    // Initialize; no smart-ref bump because lifetime is enforced by the
    // owning ForwardRenderSystem's member-destruction order.
    IBuffer* m_indirectCmd   = nullptr;
    IBuffer* m_indirectCount = nullptr;

    WRef<IPipeline> m_earlyPipeline = nullptr;
    WRef<IPipeline> m_latePipeline  = nullptr;

    ITexture::Format m_colorFormat = ITexture::Format::NONE;
};

} // namespace Yogi
