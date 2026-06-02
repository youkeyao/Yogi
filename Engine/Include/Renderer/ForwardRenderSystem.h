#pragma once

#include "Scene/SystemBase.h"
#include "Events/ApplicationEvent.h"
#include "Renderer/MeshGPUUploadCache.h"
#include "Renderer/RenderComponents.h"
#include "Renderer/DepthPyramid.h"
#include "Renderer/RenderPass.h"
#include "Renderer/ObjectCullPass.h"
#include "Renderer/MeshletDrawPass.h"
#include "Renderer/DrawSlotRegistry.h"
#include "Renderer/StagingArena.h"
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
    bool OnWindowResize(WindowResizeEvent& e, World& world);
    void ResetMeshUploadCache();
    void EnsureDepthTexture(uint32_t width, uint32_t height);

private:
    static const uint64_t MAX_TRIANGLES     = 10000000;
    static const uint64_t MAX_VERTICES      = MAX_TRIANGLES * 3;
    static const uint64_t MAX_VERTICES_SIZE = MAX_VERTICES * sizeof(VertexData);
    static const uint64_t MAX_MESHLETS      = MAX_TRIANGLES / MESHLET_MAX_TRIANGLES;
    static const uint64_t MAX_MESHLET_SIZE  = MAX_MESHLETS * sizeof(MeshletData);
    static const uint64_t MAX_MESHLET_DATA_SIZE =
        MAX_MESHLETS * (MESHLET_MAX_VERTICES + MESHLET_MAX_TRIANGLES * 3) * sizeof(uint32_t);
    static const uint64_t MAX_MATERIALS     = 65536;
    static const uint64_t MAX_MATERIAL_SIZE = MAX_MATERIALS * sizeof(MaterialData);

    WRef<IBuffer> m_vertexStorageBuffer = nullptr;
    WRef<IBuffer> m_meshletBuffer       = nullptr;
    WRef<IBuffer> m_meshletDataBuffer   = nullptr;
    WRef<IBuffer> m_meshBuffer          = nullptr;
    WRef<IBuffer> m_meshDrawBuffer      = nullptr;
    WRef<IBuffer> m_materialBuffer      = nullptr;

    StagingArena    m_stagingArena;
    ObjectCullPass  m_objectCullPass;
    MeshletDrawPass m_meshletDrawPass;

    MeshGPUUploadCache m_meshUploadCache;

    DrawSlotRegistry m_drawSlotRegistry;

    // DepthReduce pipeline + Hi-Z manager.
    WRef<IPipeline> m_depthReducePipeline = nullptr;
    DepthPyramid    m_depthPyramid;

    static constexpr float k_zNear = 0.1f;
    static constexpr float k_zFar  = 100.0f;

    WRef<ITexture>     m_depthTexture = nullptr;
    WRef<ITextureView> m_depthView    = nullptr;
    ITexture::Format   m_depthFormat  = ITexture::Format::D32_FLOAT;

    std::unordered_map<ITexture*, Owner<ITextureView>> m_materialViews;
};

} // namespace Yogi
