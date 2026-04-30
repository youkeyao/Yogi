#pragma once

#include "Scene/SystemBase.h"
#include "Events/ApplicationEvent.h"
#include "Renderer/MeshGPUUploadCache.h"
#include "Renderer/RenderComponents.h"
#include "Renderer/DepthPyramid.h"
#include "Renderer/RHI/ICommandBuffer.h"
#include "Renderer/RHI/IFrameBuffer.h"

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
    void BeginRender(ICommandBuffer* commandBuffer, const IFrameBuffer* frameBuffer);
    void EndRender(ICommandBuffer* commandBuffer);
    void ResetMeshUploadCache();
    void EnsureDepthTexture(uint32_t width, uint32_t height);
    void FlushBatch(ICommandBuffer*                    commandBuffer,
                    const IFrameBuffer*                frameBuffer,
                    ITexture*                          blitTarget,
                    CullData&                          cullData,
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
    static const uint64_t MAX_MESH_DRAWS                 = 1000000;
    static const uint64_t MAX_MESH_SIZE                  = MAX_MESH_DRAWS * sizeof(MeshData);
    static const uint64_t MAX_MESH_DRAW_SIZE             = MAX_MESH_DRAWS * sizeof(MeshDraw);
    static const uint64_t MAX_INDIRECT_DRAW_COMMAND_SIZE = MAX_MESH_DRAWS * sizeof(uint32_t) * 3;
    static const uint64_t MAX_VISIBLE_DRAW_INDEX_SIZE    = MAX_MESH_DRAWS * sizeof(uint32_t);
    static const uint64_t MAX_INDIRECT_DRAW_COUNT_SIZE   = MAX_MESH_DRAWS * sizeof(uint32_t);

    SceneData m_sceneData;

    WRef<IBuffer> m_vertexStorageBuffer         = nullptr;
    WRef<IBuffer> m_meshletBuffer               = nullptr;
    WRef<IBuffer> m_meshletDataBuffer           = nullptr;
    WRef<IBuffer> m_meshBuffer                  = nullptr;
    WRef<IBuffer> m_meshDrawBuffer              = nullptr;
    WRef<IBuffer> m_meshTaskIndirectBuffer      = nullptr;
    WRef<IBuffer> m_visibleDrawIndexBuffer      = nullptr;
    WRef<IBuffer> m_meshTaskIndirectCountBuffer = nullptr;

    MeshGPUUploadCache m_meshUploadCache;
    uint32_t           m_cachedVertexCount     = 0;
    uint32_t           m_cachedMeshletCount    = 0;
    uint32_t           m_cachedMeshletDataSize = 0;
    uint32_t           m_cachedMeshCount       = 0;

    std::vector<WRef<IRenderPass>> m_renderPasses;
    WRef<IShaderResourceBinding>   m_shaderResourceBinding     = nullptr;
    WRef<IShaderResourceBinding>   m_cullShaderResourceBinding = nullptr;
    WRef<IPipeline>                m_cullPipeline              = nullptr;
    WRef<IPipeline>                m_depthReducePipeline       = nullptr;
    DepthPyramid                   m_depthPyramid;

    WRef<ITexture>   m_depthTexture = nullptr;
    ITexture::Format m_depthFormat  = ITexture::Format::D32_FLOAT;

    std::unordered_map<uint64_t, WRef<IFrameBuffer>> m_frameBuffers;
};

} // namespace Yogi