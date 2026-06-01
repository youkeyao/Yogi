#pragma once

#include "Renderer/RenderPass.h"
#include "Renderer/RHI/IPipeline.h"

namespace Yogi
{

class StagingArena;

// Two-phase GPU object cull. Owns:
//   - EARLY / LATE compute pipelines (compiled twice off ObjectCull.comp via
//     the "::LATE=1" key suffix)
//   - Indirect command / count / visible-draw-index buffers (single-region
//     dense write -- EARLY and LATE reuse the same region; count is cleared
//     between phases)
//   - Object visibility bitfield (double-buffered, bit-packed: 1 bit per
//     MeshDraw)
//   - Meshlet visibility bitfield (double-buffered, bit-packed: 1 bit per
//     globalMeshletId)
//
// Per-frame zero-clears of the count slot + this-frame Write bitfields use
// ICommandBuffer::FillBuffer (a thin wrapper over vkCmdFillBuffer) -- no
// staging buffer needed.
//
// Frame flow:
//     BeginFrame(slot)                                  // host: swap Read/Write idx
//     FillSceneFrame(sf)                                // host: BDA pointers into sf
//     PrepareEarly(cmd)                                 // GPU: zero count + zero new Write bitfields
//     ExecuteEarly(cmd, sf, drawBase, drawCount)        // GPU: EARLY cull dispatch
//     -- caller renders EARLY --
//     -- caller builds Hi-Z --
//     PrepareLate(cmd)                                  // GPU: zero count again
//     ExecuteLate(cmd, sf, drawBase, drawCount, pyramidSlot)  // GPU: LATE cull dispatch
//     -- caller renders LATE --
//
// Multi-batch (currently unused): drawBase/drawCount let the caller break a
// frame's MeshDraws into multiple sub-batches; the same indirect/vis buffers
// are shared. Single-pipeline scenes (the common case) use drawBase=0,
// drawCount=totalDraws and a single ExecuteEarly + ExecuteLate.
class YG_API ObjectCullPass : public RenderPass
{
public:
    // Sized to match the previous ForwardRenderSystem hard caps. Public so
    // ForwardRenderSystem can size its dependent uploads (MeshDraw buffer
    // primarily).
    static constexpr uint64_t MAX_MESH_DRAWS              = 1000000ull;
    static constexpr uint64_t MAX_MESHLET_VIS_COUNT       = 32ull * 1024ull * 1024ull;

    static constexpr uint64_t INDIRECT_COMMAND_BUFFER_SIZE =
        MAX_MESH_DRAWS * sizeof(uint32_t) * 3; // (groupCountX, groupCountY, groupCountZ) per draw
    static constexpr uint64_t VISIBLE_DRAW_INDEX_BUFFER_SIZE = MAX_MESH_DRAWS * sizeof(uint32_t);
    // 256 bytes -- 1 uint is enough today (single-region single counter), but
    // leaving headroom keeps the door open to multi-bucket cull dispatches.
    static constexpr uint64_t INDIRECT_COUNT_BUFFER_SIZE = 256ull;
    // Bit-packed: ceil(N / 32) uints. 1M draws -> 128 KB; 32M meshlets -> 4 MB.
    static constexpr uint64_t OBJECT_VIS_BITFIELD_SIZE  = ((MAX_MESH_DRAWS + 31ull) / 32ull) * sizeof(uint32_t);
    static constexpr uint64_t MESHLET_VIS_BITFIELD_SIZE = ((MAX_MESHLET_VIS_COUNT + 31ull) / 32ull) * sizeof(uint32_t);

    ObjectCullPass()           = default;
    ~ObjectCullPass() override = default;

    void Initialize() override;
    void Shutdown() override;

    // Swap Read / Write indices on the double-buffered vis bitfields. Does
    // not touch the GPU; PrepareEarly is responsible for zeroing the new
    // write-side buffers from a command buffer.
    void BeginFrame() override;

    // Fill the indirect / vis BDA addresses into the per-camera SceneFrame.
    void FillSceneFrame(SceneFrame& sceneFrame) override;

    // GPU-side zero of the count slot + write-side ObjectVis + write-side
    // MeshletVis. Issues a barrier into TransferDst before the copy and
    // out into UnorderedAccess after.
    void PrepareEarly(ICommandBuffer* cmd);

    // EARLY cull dispatch. drawBase = 0 in single-pipeline mode (kept as a
    // real parameter so a multi-bucket caller can split a frame's MeshDraws
    // across multiple cull dispatches without changing this API).
    void ExecuteEarly(ICommandBuffer* cmd,
                      uint64_t        sceneFrameAddr,
                      uint32_t        drawBase,
                      uint32_t        drawCount);

    // GPU-side zero of just the count slot, between EARLY and LATE.
    void PrepareLate(ICommandBuffer* cmd);

    // LATE cull dispatch. pyramidSlot is the bindless slot of the Hi-Z
    // sampled view (built by DepthPyramid).
    void ExecuteLate(ICommandBuffer* cmd,
                     uint64_t        sceneFrameAddr,
                     uint32_t        drawBase,
                     uint32_t        drawCount,
                     uint32_t        pyramidSlot);

    // Accessors used by MeshletDrawPass to issue the indirect mesh draws.
    IBuffer* GetIndirectCommandBuffer() const { return m_indirectCommandBuffer.Get(); }
    IBuffer* GetIndirectCountBuffer() const { return m_indirectCountBuffer.Get(); }
    IBuffer* GetVisibleDrawIndexBuffer() const { return m_visibleDrawIndexBuffer.Get(); }

private:
    static const char* k_objectCullShaderEarly;
    static const char* k_objectCullShaderLate;

    // EARLY/LATE compiled twice off the same source.
    WRef<IPipeline> m_cullPipelineEarly = nullptr;
    WRef<IPipeline> m_cullPipelineLate  = nullptr;

    WRef<IBuffer> m_indirectCommandBuffer  = nullptr;
    WRef<IBuffer> m_indirectCountBuffer    = nullptr;
    WRef<IBuffer> m_visibleDrawIndexBuffer = nullptr;

    // Double-buffered, bit-packed visibility bitfields. m_visReadIdx flips
    // each BeginFrame; ObjectVisBufferRead = m_objectVis[m_visReadIdx],
    // ObjectVisBufferWrite = m_objectVis[1 - m_visReadIdx]. Same for meshlet.
    WRef<IBuffer> m_objectVis[2]  = { nullptr, nullptr };
    WRef<IBuffer> m_meshletVis[2] = { nullptr, nullptr };
    uint32_t      m_visReadIdx    = 0;
};

} // namespace Yogi
