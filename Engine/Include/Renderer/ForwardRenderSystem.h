#pragma once

#include "Scene/SystemBase.h"
#include "Events/ApplicationEvent.h"
#include "Renderer/MeshGPUUploadCache.h"
#include "Renderer/RenderComponents.h"
#include "Renderer/DepthPyramid.h"
#include "Renderer/FrameUploadArena.h"
#include "Renderer/RHI/ICommandBuffer.h"

namespace Yogi
{

class YG_API ForwardRenderSystem : public SystemBase
{
public:
    ForwardRenderSystem();
    virtual ~ForwardRenderSystem();

    void OnUpdate(Timestep ts, World& world) override;
    void OnEvent(Event& e, World& world) override;

    void RenderCamera(const CameraComponent& camera, const TransformComponent& transform, World& world);

private:
    struct RenderBatch
    {
        std::vector<MeshDraw> MeshDraws;
        uint32_t              DrawBase            = 0;
        uint32_t              DrawCount           = 0;
        uint32_t              IndirectOffsetBytes = 0;
    };

    bool OnWindowResize(WindowResizeEvent& e, World& world);
    void ResetMeshUploadCache();
    void EnsureDepthTexture(uint32_t width, uint32_t height);
    void FlushBatch(ICommandBuffer*           commandBuffer,
                    ITextureView*             colorView,
                    bool                      transitionToPresent,
                    SceneFrame&               sceneFrame,
                    std::vector<RenderBatch>& renderBatches,
                    std::vector<uint8_t>&     uploadMaterialBytes);

private:
    static const uint64_t MAX_TRIANGLES     = 10000000;
    static const uint64_t MAX_VERTICES      = MAX_TRIANGLES * 3;
    static const uint64_t MAX_VERTICES_SIZE = MAX_VERTICES * sizeof(VertexData);
    static const uint64_t MAX_MESHLETS      = MAX_TRIANGLES / MESHLET_MAX_TRIANGLES;
    static const uint64_t MAX_MESHLET_SIZE  = MAX_MESHLETS * sizeof(MeshletData);
    static const uint64_t MAX_MESHLET_DATA_SIZE =
        MAX_MESHLETS * (MESHLET_MAX_VERTICES + MESHLET_MAX_TRIANGLES * 3) * sizeof(uint32_t);
    static const uint64_t MAX_MESH_DRAWS     = 1000000;
    static const uint64_t MAX_MESH_SIZE      = MAX_MESH_DRAWS * sizeof(MeshData);
    static const uint64_t MAX_MESH_DRAW_SIZE = MAX_MESH_DRAWS * sizeof(MeshDraw);
    // Indirect command / visible-index / count buffers hold BOTH the EARLY and LATE
    // passes' output in non-overlapping halves: [0, MAX_MESH_DRAWS) is EARLY and
    // [MAX_MESH_DRAWS, 2*MAX_MESH_DRAWS) is LATE. Count buffer similarly has 2 slots
    // per batch (early then late). Doubling the size keeps per-batch offsets the same
    // as the single-pass version while letting both passes coexist.
    static const uint64_t MAX_INDIRECT_DRAW_COMMAND_SIZE = 2 * MAX_MESH_DRAWS * sizeof(uint32_t) * 3;
    static const uint64_t MAX_VISIBLE_DRAW_INDEX_SIZE    = 2 * MAX_MESH_DRAWS * sizeof(uint32_t);
    static const uint64_t MAX_INDIRECT_DRAW_COUNT_SIZE   = 2 * MAX_MESH_DRAWS * sizeof(uint32_t);
    // One uint of "was visible last frame" per MeshDraw slot. Persistent across frames,
    // never explicitly cleared (GPU buffers zero-init on allocation, which matches the
    // niagara semantics of "unseen = hasn't been visible yet = go through LATE pass").
    static const uint64_t MAX_OBJECT_VIS_SIZE = MAX_MESH_DRAWS * sizeof(uint32_t);
    // Per-instance per-meshlet visibility. Sized for sum(MeshletCount) across all
    // MeshDraws uploaded this frame. The Sandbox scene with 10K instances of
    // Armadillo/Bunny (a few thousand meshlets each) needs ~12M entries; pad to
    // 32M (128 MB) for headroom. Cap is checked at upload time -- overflow aborts
    // the frame to prevent the LATE task shader from writing past the buffer end
    // (BDA, no descriptor bounds check, would corrupt adjacent device memory and
    // poison fences/semaphores). Each entry is one uint of "was emitted (passed
    // full cull) last frame", same semantic as the per-object ObjectVisBuffer
    // but indexed by globalMeshletId = draw.MeshletVisOffset + miLocal.
    // Persistent across frames.
    static const uint64_t MAX_MESHLET_VIS_COUNT = 32ull * 1024ull * 1024ull;
    static const uint64_t MAX_MESHLET_VIS_SIZE  = MAX_MESHLET_VIS_COUNT * sizeof(uint32_t);
    // Per-frame uploaded MaterialData -- one slot per unique Material referenced
    // this frame. Cap is generous (64K materials per flush); a real scene with
    // 10K instances of 1 Material uses 1 slot.
    static const uint64_t MAX_MATERIALS     = 65536;
    static const uint64_t MAX_MATERIAL_SIZE = MAX_MATERIALS * sizeof(MaterialData);

    // Niagara-style mostly-bindless rendering. Cull / mesh / frag pipelines
    // declare a single descriptor set (the engine's BindlessTextures SRB) for
    // sampled textures + immutable samplers; per-pipeline state flows through
    // push constants and BDA-addressed buffers. The DepthReduce pipeline is
    // the one exception -- it owns private storage-image write targets and
    // their immediate sampled sources, so it brings its own SRB chain (one
    // per mip) instead of going through BindlessTextures.

    WRef<IBuffer> m_vertexStorageBuffer         = nullptr;
    WRef<IBuffer> m_meshletBuffer               = nullptr;
    WRef<IBuffer> m_meshletDataBuffer           = nullptr;
    WRef<IBuffer> m_meshBuffer                  = nullptr;
    WRef<IBuffer> m_meshDrawBuffer              = nullptr;
    WRef<IBuffer> m_meshTaskIndirectBuffer      = nullptr;
    WRef<IBuffer> m_visibleDrawIndexBuffer      = nullptr;
    WRef<IBuffer> m_meshTaskIndirectCountBuffer = nullptr;
    // Persistent per-draw visibility[] for niagara-style two-phase culling. Aliased as
    // both "prev" (binding 6) and "curr" (binding 7) in ObjectCull.comp -- a compute-to-
    // compute barrier between EARLY and LATE dispatches is what makes the single-buffer
    // scheme correct.
    WRef<IBuffer> m_objectVisBuffer = nullptr;
    // Per-instance per-meshlet visibility (Alan Wake 2 / Northlight style). EARLY task
    // reads only ("emit if prev_meshlet_vis==1 AND cone+frustum"), LATE task reads then
    // writes the full this-frame pass result so next frame's EARLY has up-to-date state.
    // Object LATE cull dispatches all currently-visible objects (not just newly-visible
    // ones), so even stable-visible objects' meshlet vis gets refreshed every frame.
    WRef<IBuffer> m_meshletVisBuffer = nullptr;
    // Bindless material data. Uploaded each frame from RenderCamera's collect
    // loop -- per-Material (not per-instance), so 10K instances of one Material
    // produce one entry. MeshDraw.MaterialIndex is the index into this buffer.
    WRef<IBuffer> m_materialBuffer = nullptr;
    // Per-frame static data (matrices + buffer pointers) for both the mesh/task
    // and cull shaders. Push constants only carry the BDA returned by the arena
    // plus per-batch state. The arena is a single host-visible buffer carved into
    // GetImageCount() segments, so frame N's host writes never race frame N-1's
    // GPU reads -- equivalent to the previous "one IBuffer per slot per struct"
    // scheme but with one VkBuffer instead of (2 * imageCount) of them, and with
    // room to grow for any future per-frame structs at zero extra allocation.
    FrameUploadArena m_frameArena;

    MeshGPUUploadCache m_meshUploadCache;
    uint32_t           m_cachedVertexCount     = 0;
    uint32_t           m_cachedMeshletCount    = 0;
    uint32_t           m_cachedMeshletDataSize = 0;
    uint32_t           m_cachedMeshCount       = 0;
    // Parallel to m_meshUploadCache (indexed by meshIndex). Lets FlushBatch compute
    // the per-MeshDraw MeshletVisOffset prefix sum without reading back the
    // GPU-side MeshDataBuffer. Reset alongside the upload cache when it spills.
    std::vector<uint32_t> m_cachedMeshMeshletCounts;

    // Two compile-time variants of ObjectCull.comp:
    //   EARLY -- prev_obj_vis==1 gate, no Hi-Z, no vis writes.
    //   LATE  -- full cone+frustum+Hi-Z, writes vis for next frame's EARLY.
    WRef<IPipeline> m_cullPipelineEarly = nullptr;
    WRef<IPipeline> m_cullPipelineLate  = nullptr;
    // Meshlet (task + mesh + frag) graphics pipelines. Single pair shared by
    // every MeshRenderer -- Material is pure data and doesn't carry pipelines.
    // EARLY / LATE are compiled twice off the same Test.task source via the
    // "::LATE=1" key suffix; LATE adds Hi-Z occlusion + meshlet vis updates.
    WRef<IPipeline> m_meshletEarlyPipeline = nullptr;
    WRef<IPipeline> m_meshletLatePipeline  = nullptr;
    WRef<IPipeline> m_depthReducePipeline  = nullptr;
    DepthPyramid    m_depthPyramid;

    // Scene znear/zfar match MathUtils::Perspective(..., 0.1f, 100.0f) in RenderCamera().
    static constexpr float k_zNear = 0.1f;
    static constexpr float k_zFar  = 100.0f;

    WRef<ITexture>     m_depthTexture = nullptr;
    WRef<ITextureView> m_depthView    = nullptr;
    ITexture::Format   m_depthFormat  = ITexture::Format::D32_FLOAT;

    // Cache of sampled views keyed by ITexture* -- one view per unique
    // texture referenced by any live Material. Each view auto-registers a
    // bindless slot in its ctor; we hold the Owner here so the slot stays
    // live across frames (Materials that re-reference the same texture
    // tomorrow don't pay the re-registration cost). Cleared on shutdown
    // along with everything else.
    std::unordered_map<ITexture*, Owner<ITextureView>> m_materialViews;
};

} // namespace Yogi
