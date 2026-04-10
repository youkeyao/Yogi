#pragma once

#include "Scene/SystemBase.h"
#include "Events/ApplicationEvent.h"
#include "Renderer/MeshGPUUploadCache.h"
#include "Renderer/RenderComponents.h"
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
    bool OnWindowResize(WindowResizeEvent& e, World& world);
    void BeginRender(View<ICommandBuffer> commandBuffer, View<IFrameBuffer> frameBuffer);
    void EndRender(View<ICommandBuffer> commandBuffer);
    void EnsureDepthPyramidResources(View<ICommandBuffer> commandBuffer,
                                     uint32_t             width,
                                     uint32_t             height,
                                     View<ITexture>       depthTexture);
    void BuildDepthPyramid(View<ICommandBuffer> commandBuffer, View<ITexture> depthTexture);
    void ResetMeshUploadCache();

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

    std::vector<WRef<IRenderPass>>                              m_renderPasses;
    WRef<IShaderResourceBinding>                                m_shaderResourceBinding     = nullptr;
    WRef<IShaderResourceBinding>                                m_cullShaderResourceBinding = nullptr;
    Owner<IShaderResourceBinding>                               m_depthPyramidMipBinding;
    std::vector<Owner<IShaderResourceBinding>>                  m_depthPyramidMipBindings;
    std::unordered_map<uint64_t, Owner<IShaderResourceBinding>> m_depthPyramidFirstMipBindingCache;
    WRef<IPipeline>                                             m_cullPipeline                 = nullptr;
    WRef<IPipeline>                                             m_depthPyramidFirstMipPipeline = nullptr;
    WRef<IPipeline>                                             m_depthPyramidPipeline         = nullptr;

    WRef<ITexture> m_depthPyramidTexture = nullptr;
    uint32_t       m_depthPyramidWidth   = 0;
    uint32_t       m_depthPyramidHeight  = 0;
    uint32_t       m_depthPyramidMips    = 1;
    bool           m_depthPyramidValid   = false;

    std::unordered_map<uint64_t, WRef<IFrameBuffer>> m_frameBuffers;
};

} // namespace Yogi