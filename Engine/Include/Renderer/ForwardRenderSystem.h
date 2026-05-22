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
        uint64_t              PipelineKey = 0;
        WRef<IPipeline>       Pipeline;
        std::vector<MeshDraw> MeshDraws;
        uint32_t              DrawBase            = 0;
        uint32_t              DrawCount           = 0;
        uint32_t              IndirectOffsetBytes = 0;
    };

    bool OnWindowResize(WindowResizeEvent& e, World& world);
    void ResetMeshUploadCache();
    void EnsureDepthTexture(uint32_t width, uint32_t height);
    void FlushBatch(ICommandBuffer*                    commandBuffer,
                    ITextureView*                      colorView,
                    bool                               transitionToPresent,
                    SceneFrame&                        sceneFrame,
                    CullFrame&                         cullFrame,
                    std::vector<RenderBatch>&          renderBatches,
                    std::unordered_map<uint64_t, int>& renderBatchLookup);

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
    static const uint64_t MAX_VISIBILITY_SIZE = MAX_MESH_DRAWS * sizeof(uint32_t);

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
    WRef<IBuffer> m_visibilityBuffer = nullptr;
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

    WRef<IShaderResourceBinding> m_shaderResourceBinding     = nullptr;
    WRef<IShaderResourceBinding> m_cullShaderResourceBinding = nullptr;
    WRef<IPipeline>              m_cullPipeline              = nullptr;
    WRef<IPipeline>              m_depthReducePipeline       = nullptr;
    DepthPyramid                 m_depthPyramid;

    // Scene znear/zfar match MathUtils::Perspective(..., 0.1f, 100.0f) in RenderCamera().
    static constexpr float k_zNear = 0.1f;
    static constexpr float k_zFar  = 100.0f;

    WRef<ITexture>     m_depthTexture = nullptr;
    WRef<ITextureView> m_depthView    = nullptr;
    ITexture::Format   m_depthFormat  = ITexture::Format::D32_FLOAT;
};

} // namespace Yogi
