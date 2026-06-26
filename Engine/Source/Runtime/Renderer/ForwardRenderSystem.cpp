#include "Renderer/ForwardRenderSystem.h"
#include "Renderer/Material.h"
#include "Renderer/BindlessTextureManager.h"
#include "Resources/AssetManager/AssetManager.h"
#include "Resources/ResourceManager/ResourceManager.h"
#include "Core/Application.h"
#include "Core/Input.h"
#include "Events/KeyCodes.h"
#include "Scene/World.h"
#include "Math/Vector.h"

namespace Yogi
{

ForwardRenderSystem::ForwardRenderSystem()
{
    m_vertexStorageBuffer =
        ResourceManager::AcquireSharedResource<IBuffer>(BufferDesc{ MAX_VERTICES_SIZE, BufferUsage::Storage });
    m_meshletBuffer =
        ResourceManager::AcquireSharedResource<IBuffer>(BufferDesc{ MAX_MESHLET_SIZE, BufferUsage::Storage });
    m_meshletDataBuffer =
        ResourceManager::AcquireSharedResource<IBuffer>(BufferDesc{ MAX_MESHLET_DATA_SIZE, BufferUsage::Storage });
    m_meshBuffer = ResourceManager::AcquireSharedResource<IBuffer>(
        BufferDesc{ MAX_MESH_DRAWS * sizeof(MeshData), BufferUsage::Storage });
    m_meshDrawBuffer = ResourceManager::AcquireSharedResource<IBuffer>(
        BufferDesc{ MAX_MESH_DRAWS * sizeof(MeshDraw), BufferUsage::Storage });
    m_drawIndexBuffer = ResourceManager::AcquireSharedResource<IBuffer>(
        BufferDesc{ MAX_MESH_DRAWS * sizeof(uint32_t), BufferUsage::Storage });

    // ---- StagingArena ----
    {
        constexpr uint64_t kBlockSize = 1ull * 1024ull * 1024ull;
        m_stagingArena.Init(kBlockSize);
    }

    m_drawSlotRegistry.Init(static_cast<uint32_t>(MAX_MESH_DRAWS),
                            static_cast<uint32_t>(ObjectCull::MAX_MESHLET_VIS_COUNT));

    m_renderGraph.RegisterBuffer("Geometry.Vertex", m_vertexStorageBuffer);
    m_renderGraph.RegisterBuffer("Geometry.Meshlet", m_meshletBuffer);
    m_renderGraph.RegisterBuffer("Geometry.MeshletData", m_meshletDataBuffer);
    m_renderGraph.RegisterBuffer("Geometry.MeshData", m_meshBuffer);
    m_renderGraph.RegisterBuffer("Geometry.MeshDraw", m_meshDrawBuffer);
    m_renderGraph.RegisterBuffer("Geometry.DrawIndex", m_drawIndexBuffer);

    // ---- Passes ----
    m_renderGraph.RegisterPass<ObjectCullClearPass>();
    m_renderGraph.RegisterPass<ObjectCullEarlyPass>();
    m_renderGraph.RegisterPass<MeshletDrawEarlyPass>();
    m_renderGraph.RegisterPass<HiZPass>();
    m_renderGraph.RegisterPass<ObjectCullLatePass>();
    m_renderGraph.RegisterPass<MeshletDrawLatePass>();
    m_renderGraph.RegisterPass<OutlinePass>();
}

ForwardRenderSystem::~ForwardRenderSystem()
{
    ResetMeshUploadCache();

    m_renderGraph.Clear();

    m_vertexStorageBuffer = nullptr;
    m_meshletBuffer       = nullptr;
    m_meshletDataBuffer   = nullptr;
    m_meshBuffer          = nullptr;
    m_meshDrawBuffer      = nullptr;
    m_drawIndexBuffer     = nullptr;

    m_stagingArena.Shutdown();

    m_depthView    = nullptr;
    m_depthTexture = nullptr;

    if (BindlessTextureManager::IsInitialized())
    {
        for (auto& [tex, view] : m_materialViews)
        {
            const uint32_t slot = BindlessTextureManager::Find(view.Get());
            if (slot != BindlessTextureManager::INVALID_SLOT)
                BindlessTextureManager::Unregister(slot);
        }
    }
    m_materialViews.clear();
}

void ForwardRenderSystem::OnUpdate(Timestep ts, World& world)
{
    world.ViewComponents<TransformComponent, CameraComponent>(
        [&](Entity entity, TransformComponent& transform, CameraComponent& camera) {
            RenderCamera(camera, transform, world);
        });
}

void ForwardRenderSystem::OnEvent(Event& e, World& world)
{
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowResizeEvent>(YG_BIND_FN(ForwardRenderSystem::OnWindowResize, std::placeholders::_2),
                                           world);
}

bool ForwardRenderSystem::OnWindowResize(WindowResizeEvent& e, World& world)
{
    auto swapChain = Application::GetInstance().GetSwapChain();
    swapChain->AcquireCurrentCommandBuffer()->Wait();
    m_depthView    = nullptr;
    m_depthTexture = nullptr;
    return false;
}

void ForwardRenderSystem::ResetMeshUploadCache()
{
    m_meshUploadCache.Clear();
    m_drawSlotRegistry.Reset();
}

void ForwardRenderSystem::EnsureDepthTexture(uint32_t width, uint32_t height)
{
    if (m_depthTexture && m_depthTexture->GetWidth() == width && m_depthTexture->GetHeight() == height)
        return;

    TextureDesc desc{};
    desc.Width      = width;
    desc.Height     = height;
    desc.MipLevels  = 1;
    desc.Format     = m_depthFormat;
    desc.NumSamples = SampleCountFlagBits::Count1;
    desc.UsageFlags = TextureUsageFlags::DepthStencil | TextureUsageFlags::Sampled;
    m_depthTexture  = ResourceManager::CreateResource<ITexture>(desc);
    m_depthView     = ResourceManager::CreateResource<ITextureView>(m_depthTexture, TextureViewDesc{});
}

void ForwardRenderSystem::RenderCamera(const CameraComponent& camera, const TransformComponent& transform, World& world)
{
    YG_PROFILE_FUNCTION();

    auto            swapChain     = Application::GetInstance().GetSwapChain();
    ICommandBuffer* commandBuffer = swapChain->AcquireCurrentCommandBuffer().Get();
    ITextureView*   currentTarget = swapChain->AcquireCurrentTarget().Get();
    if (camera.Target)
    {
        currentTarget = camera.Target.Get();
    }
    const ITexture* targetTex = currentTarget->GetTexture();
    EnsureDepthTexture(targetTex->GetWidth(), targetTex->GetHeight());

    const bool transitionToPresent = !camera.Target;

    // Per-frame begin (host-side resets).
    commandBuffer->Wait();
    m_stagingArena.BeginFrame();

    // ---- Build the per-camera SceneFrame ----
    Matrix4 viewMatrix = MathUtils::Inverse(transform.Transform);
    Matrix4 projectionMatrix;
    float   aspectRatio = (float)targetTex->GetWidth() / (float)targetTex->GetHeight();
    if (camera.IsOrtho)
    {
        projectionMatrix = MathUtils::Orthographic(-aspectRatio * camera.ZoomLevel,
                                                   aspectRatio * camera.ZoomLevel,
                                                   -camera.ZoomLevel,
                                                   camera.ZoomLevel,
                                                   -1.0f,
                                                   1.0f);
    }
    else
    {
        projectionMatrix = MathUtils::Perspective(camera.Fov, aspectRatio, k_zNear, k_zFar);
    }
    SceneFrame sceneFrame{};
    sceneFrame.ProjectionViewMatrix = projectionMatrix * viewMatrix;
    sceneFrame.ViewMatrix           = viewMatrix;
    sceneFrame.ScreenSize = { static_cast<float>(targetTex->GetWidth()), static_cast<float>(targetTex->GetHeight()) };

    auto    PT         = projectionMatrix.Transpose();
    Vector4 leftRaw    = PT[3] + PT[0];
    Vector4 topRaw     = PT[3] + PT[1];
    Vector3 leftN      = Vector3(leftRaw.x, leftRaw.y, leftRaw.z);
    Vector3 topN       = Vector3(topRaw.x, topRaw.y, topRaw.z);
    float   leftLen    = leftN.Length();
    float   topLen     = topN.Length();
    Vector4 left       = leftLen > 0.0f ? leftRaw / leftLen : leftRaw;
    Vector4 top        = topLen > 0.0f ? topRaw / topLen : topRaw;
    sceneFrame.Frustum = Vector4(left.x, -left.z, top.y, -top.z);

    sceneFrame.P00   = projectionMatrix[0][0];
    sceneFrame.P11   = projectionMatrix[1][1];
    sceneFrame.ZNear = k_zNear;
    sceneFrame.ZFar  = k_zFar;

    // ---- Begin recording. Stage uploads BEFORE cull dispatch. ----
    commandBuffer->Begin();
    commandBuffer->BeginDebugLabel("ForwardRenderSystem::RenderCamera");

    // ============== Collect mesh data + slot-aware MeshDraw build ==============
    m_drawSlotRegistry.BeginFrame();
    m_slabUploader.BeginFrame();
    std::vector<DrawSlotRegistry::DirtyEntry> dirtyDraws;
    struct LiveDraw
    {
        uint32_t    BufferIndex;
        std::string ShaderKey;
    };
    std::vector<LiveDraw> liveDraws;
    liveDraws.reserve(256);

    MaterialSchema::TextureResolver textureResolver = [this](const WRef<ITexture>& tex) -> uint32_t {
        if (!tex)
            return 0u;
        ITexture* key = tex.Get();
        auto      it  = m_materialViews.find(key);
        if (it == m_materialViews.end())
        {
            auto inserted = m_materialViews.emplace(key, ITextureView::Create(tex));
            it            = inserted.first;
        }
        return BindlessTextureManager::Register(it->second.Get());
    };

    bool overflowed = false;
    world.ViewComponents<TransformComponent, MeshRendererComponent>(
        [&](Entity entity, TransformComponent& meshTransform, MeshRendererComponent& meshRenderer) {
            if (overflowed)
                return;

            auto& mesh     = meshRenderer.Mesh;
            auto& material = meshRenderer.Material;
            if (!mesh || !material)
                return;

            uint32_t meshVertexCount  = static_cast<uint32_t>(mesh->GetVertices().size());
            uint32_t meshMeshletCount = static_cast<uint32_t>(mesh->GetMeshlets().size());

            std::string assetKey = AssetManager::GetAssetKey(mesh);
            auto        meshKey  = MeshGPUUploadCache::BuildKey(mesh, assetKey, 0);

            uint32_t meshIndex = m_meshUploadCache.MeshCount();
            if (!m_meshUploadCache.TryGet(meshKey, meshIndex))
            {
                uint32_t meshletDataSize = static_cast<uint32_t>(mesh->GetMeshletData().size());

                if (m_meshUploadCache.VertexCount() + meshVertexCount > MAX_VERTICES ||
                    m_meshUploadCache.MeshletCount() + meshMeshletCount > MAX_MESHLETS ||
                    m_meshUploadCache.MeshletDataSize() + meshletDataSize > MAX_MESHLET_DATA_SIZE / sizeof(uint32_t))
                {
                    YG_CORE_ERROR("ForwardRenderSystem: per-mesh upload cache cap exceeded mid-frame. Skipping rest.");
                    overflowed = true;
                    return;
                }

                MeshData meshData{};
                meshData.BoundingCenter  = mesh->GetCenter();
                meshData.BoundingRadius  = mesh->GetBoundingRadius();
                meshData.VertexOffset    = m_meshUploadCache.VertexCount();
                meshData.MeshletOffset   = m_meshUploadCache.MeshletCount();
                meshData.MeshletCount    = meshMeshletCount;
                meshData.MeshletDataBase = m_meshUploadCache.MeshletDataSize();
                m_stagingArena.Stage(commandBuffer,
                                     m_meshBuffer.Get(),
                                     static_cast<uint64_t>(meshIndex) * sizeof(MeshData),
                                     &meshData,
                                     sizeof(MeshData));
                if (!mesh->GetVertices().empty())
                {
                    m_stagingArena.Stage(commandBuffer,
                                         m_vertexStorageBuffer.Get(),
                                         static_cast<uint64_t>(meshData.VertexOffset) * sizeof(VertexData),
                                         mesh->GetVertices().data(),
                                         mesh->GetVertices().size() * sizeof(VertexData));
                }
                if (!mesh->GetMeshlets().empty())
                {
                    m_stagingArena.Stage(commandBuffer,
                                         m_meshletBuffer.Get(),
                                         static_cast<uint64_t>(meshData.MeshletOffset) * sizeof(MeshletData),
                                         mesh->GetMeshlets().data(),
                                         mesh->GetMeshlets().size() * sizeof(MeshletData));
                }
                if (!mesh->GetMeshletData().empty())
                {
                    m_stagingArena.Stage(commandBuffer,
                                         m_meshletDataBuffer.Get(),
                                         static_cast<uint64_t>(meshData.MeshletDataBase) * sizeof(uint32_t),
                                         mesh->GetMeshletData().data(),
                                         mesh->GetMeshletData().size() * sizeof(uint32_t));
                }

                m_meshUploadCache.Advance(meshVertexCount, meshMeshletCount, meshletDataSize);

                m_meshUploadCache.Upsert(meshKey, meshIndex);
            }

            const uint32_t entityHandle = static_cast<uint32_t>(entity);
            uint32_t       bufferIndex  = 0;
            uint32_t       visOffset    = 0;

            const DrawSlotRegistry::AcquireResult acquire =
                m_drawSlotRegistry.Acquire(entityHandle, meshIndex, meshMeshletCount, bufferIndex, visOffset);
            if (acquire == DrawSlotRegistry::AcquireResult::CapExceeded)
            {
                overflowed = true;
                return;
            }
            if (acquire == DrawSlotRegistry::AcquireResult::VisExhausted)
                return;

            // Build the MeshDraw with the slot's stable MeshletVisOffset.
            MeshDraw draw{};
            draw.Position    = meshTransform.Transform.Position;
            draw.Scale       = meshTransform.Transform.Scale;
            draw.Orientation = Vector4(meshTransform.Transform.Rotation.x,
                                       meshTransform.Transform.Rotation.y,
                                       meshTransform.Transform.Rotation.z,
                                       meshTransform.Transform.Rotation.w);
            draw.MeshIndex   = meshIndex;
            std::string shaderKey;
            if (material)
            {
                shaderKey = AssetManager::GetAssetKey(material->Schema);
                if (shaderKey.empty())
                    shaderKey = std::string{ Material::kDefaultMaterialSchemaKey };
            }
            const auto staged     = m_slabUploader.Stage(material.Get(), textureResolver);
            draw.MaterialIndex    = staged.SlabLocalIndex;
            draw.MeshletVisOffset = visOffset;

            // Dirty diff: only stage if anything changed since last upload.
            m_drawSlotRegistry.CommitDraw(entityHandle, draw, dirtyDraws);

            // Track this slot in the per-frame live list so the bucket
            // splitter below knows it exists, regardless of whether the
            // MeshDraw itself was dirty this frame.
            liveDraws.push_back(LiveDraw{ bufferIndex, shaderKey });
        });

    if (overflowed)
    {
        commandBuffer->EndDebugLabel();
        commandBuffer->End();
        commandBuffer->Submit();
        return;
    }

    m_drawSlotRegistry.ReclaimUnseen(dirtyDraws);
    if (!dirtyDraws.empty())
    {
        std::sort(dirtyDraws.begin(),
                  dirtyDraws.end(),
                  [](const DrawSlotRegistry::DirtyEntry& a, const DrawSlotRegistry::DirtyEntry& b) {
                      return a.BufferIndex < b.BufferIndex;
                  });

        size_t i = 0;
        while (i < dirtyDraws.size())
        {
            size_t runEnd = i + 1;
            while (runEnd < dirtyDraws.size() &&
                   dirtyDraws[runEnd].BufferIndex == dirtyDraws[runEnd - 1].BufferIndex + 1)
            {
                ++runEnd;
            }
            const size_t   runLen      = runEnd - i;
            const uint64_t dstOffset   = static_cast<uint64_t>(dirtyDraws[i].BufferIndex) * sizeof(MeshDraw);
            const uint64_t runByteSize = runLen * sizeof(MeshDraw);

            void*     mapped = m_stagingArena.BeginMappedStage(commandBuffer, runByteSize, alignof(MeshDraw));
            MeshDraw* dst    = static_cast<MeshDraw*>(mapped);
            for (size_t j = 0; j < runLen; ++j)
                dst[j] = dirtyDraws[i + j].Draw;
            m_stagingArena.CommitMappedStage(commandBuffer, m_meshDrawBuffer.Get(), dstOffset, runByteSize);

            i = runEnd;
        }
    }

    // ============== Phase 5: per-type material slabs ==============
    // Upload staged material data to per-type GPU buffers. Must happen
    // before we query GPU addresses for push constants.
    m_slabUploader.Submit(commandBuffer, m_stagingArena);

    // ============== Phase 6: bucket the live draws by shaderKey =====
    // For each (shaderKey) bucket we want a contiguous [DrawBase, DrawBase+Count)
    // range of slot indices in m_drawIndexBuffer. The cull/draw passes
    // dispatch once per bucket using that range; ObjectCull.cs.slang reads
    // pcObjectCull.DrawIndexBuffer[DrawBase + thread] to recover the real slot.
    //
    // We sort liveDraws by (shaderKey, bufferIndex). Stable order inside a
    // bucket keeps RenderDoc captures readable; it doesn't otherwise matter
    // for correctness.
    std::vector<DrawBucket> buckets;

    if (!liveDraws.empty())
    {
        std::sort(liveDraws.begin(), liveDraws.end(), [](const LiveDraw& a, const LiveDraw& b) {
            if (a.ShaderKey != b.ShaderKey)
                return a.ShaderKey < b.ShaderKey;
            return a.BufferIndex < b.BufferIndex;
        });

        std::vector<uint32_t> drawIndices;
        drawIndices.reserve(liveDraws.size());
        uint32_t cursor = 0;
        for (size_t i = 0; i < liveDraws.size();)
        {
            const std::string& t           = liveDraws[i].ShaderKey;
            uint32_t           bucketStart = cursor;
            while (i < liveDraws.size() && liveDraws[i].ShaderKey == t)
            {
                drawIndices.push_back(liveDraws[i].BufferIndex);
                ++cursor;
                ++i;
            }
            DrawBucket b{};
            b.ShaderKey          = t;
            b.DrawBase           = bucketStart;
            b.DrawCount          = cursor - bucketStart;
            b.MaterialBufferAddr = m_slabUploader.GetMaterialBufferAddr(t);
            if (b.MaterialBufferAddr == 0)
            {
                YG_CORE_WARN("ForwardRender: MaterialBufferAddr is 0 for shaderKey='{0}'", t);
            }
            buckets.push_back(b);
        }

        m_stagingArena.Stage(commandBuffer,
                             m_drawIndexBuffer.Get(),
                             /*dstOffset*/ 0,
                             drawIndices.data(),
                             drawIndices.size() * sizeof(uint32_t));
    }

    const uint64_t sceneFrameAddr = m_stagingArena.Push(commandBuffer, &sceneFrame, sizeof(SceneFrame));

    commandBuffer->Barrier(ResourceState::CopyDestination,
                           ResourceState::ComputeShaderResource | ResourceState::TaskShaderResource |
                               ResourceState::MeshShaderResource | ResourceState::FragmentShaderResource);

    const uint32_t     targetWidth  = currentTarget->GetTexture()->GetWidth();
    const uint32_t     targetHeight = currentTarget->GetTexture()->GetHeight();
    RenderGraphContext graphContext{};
    graphContext.SceneFrameAddr      = sceneFrameAddr;
    graphContext.CurrentTarget       = currentTarget;
    graphContext.DepthView           = m_depthView.Get();
    graphContext.TargetWidth         = targetWidth;
    graphContext.TargetHeight        = targetHeight;
    graphContext.Buckets             = &buckets;
    graphContext.TotalDrawCount      = m_drawSlotRegistry.GetDrawCount();
    graphContext.TransitionToPresent = transitionToPresent;
    if (transitionToPresent)
        graphContext.SetInitialTextureState(currentTarget, ResourceState::Present);

    m_renderGraph.Execute(commandBuffer, graphContext);

    commandBuffer->EndDebugLabel();
    commandBuffer->End();
    commandBuffer->Submit();
}

} // namespace Yogi
