#include "Renderer/ForwardRenderSystem.h"
#include "Renderer/Material.h"
#include "Renderer/PipelineRegistry.h"
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
                            static_cast<uint32_t>(ObjectCullPass::MAX_MESHLET_VIS_COUNT));

    // ---- Passes ----
    m_objectCullPass.Initialize();
    auto swapChain = Application::GetInstance().GetSwapChain();
    m_meshletDrawPass.SetIndirectBuffers(m_objectCullPass.AcquireIndirectCommandBuffer(),
                                         m_objectCullPass.AcquireIndirectCountBuffer());
    m_meshletDrawPass.Initialize();

    // ---- OutlinePass (Phase 7 demo) ----
    m_outlinePass.SetIndirectBuffers(m_objectCullPass.AcquireIndirectCommandBuffer(),
                                     m_objectCullPass.AcquireIndirectCountBuffer());
    m_outlinePass.Initialize();

    // ---- Register PipelineRegistry builders ----
    // Each pass owns its colorFormat decision -- GetPipelineBuilder()
    // queries the swap-chain format internally.
    m_pipelineRegistry.RegisterPass(m_meshletDrawPass.GetPassName(), m_meshletDrawPass.GetPipelineBuilder());
    m_pipelineRegistry.RegisterPass(m_outlinePass.GetPassName(), m_outlinePass.GetPipelineBuilder());

    // Pre-warm StandardMaterial
    (void)m_pipelineRegistry.Acquire(m_meshletDrawPass.GetPassName(), "EngineAssets/Shaders/Materials/Standard.slang");
    (void)m_pipelineRegistry.Acquire(m_outlinePass.GetPassName(), "EngineAssets/Shaders/Materials/Standard.slang");

    // ---- DepthPyramid pipeline ----
    WRef<IShaderResourceBinding> depthReduceBindingTemplate =
        ResourceManager::AddResource(DepthPyramid::CreateReduceBindingLayout());
    WRef<ShaderDesc> depthReduceShader =
        AssetManager::AcquireAsset<ShaderDesc>("EngineAssets/Shaders/Passes/DepthReduce.cs.slang");
    PipelineDesc depthReducePipelineDesc{};
    depthReducePipelineDesc.Type               = PipelineType::Compute;
    depthReducePipelineDesc.Shaders            = { depthReduceShader.Get() };
    depthReducePipelineDesc.ResourceBinding    = depthReduceBindingTemplate.Get();
    depthReducePipelineDesc.PushConstantRanges = DepthPyramid::GetReducePushConstantRanges();
    m_depthReducePipeline = ResourceManager::AcquireSharedResource<IPipeline>(depthReducePipelineDesc);
}

ForwardRenderSystem::~ForwardRenderSystem()
{
    ResetMeshUploadCache();

    m_meshletDrawPass.Shutdown();
    m_outlinePass.Shutdown();
    m_objectCullPass.Shutdown();

    m_vertexStorageBuffer = nullptr;
    m_meshletBuffer       = nullptr;
    m_meshletDataBuffer   = nullptr;
    m_meshBuffer          = nullptr;
    m_meshDrawBuffer      = nullptr;
    m_drawIndexBuffer     = nullptr;

    m_stagingArena.Shutdown();

    m_depthReducePipeline = nullptr;
    m_depthPyramid.Reset();
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
    m_depthPyramid.Reset();
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
    desc.Usage      = ITexture::Usage::DepthStencil;
    desc.NumSamples = SampleCountFlagBits::Count1;
    m_depthTexture  = ResourceManager::CreateResource<ITexture>(desc);
    m_depthView     = ResourceManager::CreateResource<ITextureView>(m_depthTexture);
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
    m_objectCullPass.BeginFrame();
    m_meshletDrawPass.BeginFrame();

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

    // BDA pointers owned by ForwardRenderSystem.
    sceneFrame.VertexBuffer      = m_vertexStorageBuffer->GetDeviceAddress();
    sceneFrame.MeshletBuffer     = m_meshletBuffer->GetDeviceAddress();
    sceneFrame.MeshletDataBuffer = m_meshletDataBuffer->GetDeviceAddress();
    sceneFrame.MeshDataBuffer    = m_meshBuffer->GetDeviceAddress();
    sceneFrame.MeshDrawBuffer    = m_meshDrawBuffer->GetDeviceAddress();
    sceneFrame.DrawIndexBuffer   = m_drawIndexBuffer->GetDeviceAddress();

    // BDA pointers owned by ObjectCullPass (indirect / vis double buffer).
    m_objectCullPass.FillSceneFrame(sceneFrame);

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
                (void)m_pipelineRegistry.Acquire(m_meshletDrawPass.GetPassName(), shaderKey);
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
    m_slabUploader.Submit(commandBuffer, m_stagingArena, sceneFrame);

    // ============== Phase 6: bucket the live draws by shaderKey =====
    // For each (shaderKey) bucket we want a contiguous [DrawBase, DrawBase+Count)
    // range of slot indices in m_drawIndexBuffer. The cull/draw passes
    // dispatch once per bucket using that range; ObjectCull.cs.slang reads
    // sf.DrawIndexBuffer[DrawBase + thread] to recover the real slot.
    //
    // We sort liveDraws by (shaderKey, bufferIndex). Stable order inside a
    // bucket keeps RenderDoc captures readable; it doesn't otherwise matter
    // for correctness.
    std::vector<DrawBucket> buckets;
    std::vector<std::string>
        bucketTypeNames; // parallel to buckets, used by passes that need typeName for PipelineRegistry

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

            std::string typeName = "StandardMaterial"; // default fallback
            size_t      pos      = t.find_last_of('/');
            if (pos != std::string::npos)
            {
                std::string stem = t.substr(pos + 1);
                // Remove ".slang" extension
                size_t dot = stem.find_last_of('.');
                if (dot != std::string::npos)
                    stem = stem.substr(0, dot);
                typeName = stem + "Material";
            }
            bucketTypeNames.push_back(std::move(typeName));
        }

        m_stagingArena.Stage(commandBuffer,
                             m_drawIndexBuffer.Get(),
                             /*dstOffset*/ 0,
                             drawIndices.data(),
                             drawIndices.size() * sizeof(uint32_t));
    }

    const uint64_t sceneFrameAddr = m_stagingArena.Push(commandBuffer, &sceneFrame, sizeof(SceneFrame));

    // ============== Drain transfer writes -> shader-read state ==============
    commandBuffer->Barrier(ResourceState::CopyDestination,
                           ResourceState::ComputeShaderResource | ResourceState::TaskShaderResource |
                               ResourceState::MeshShaderResource | ResourceState::FragmentShaderResource);

    const uint32_t totalDrawCount = m_drawSlotRegistry.GetDrawCount();

    m_depthPyramid.Resize(m_depthTexture->GetWidth(), m_depthTexture->GetHeight());

    const uint32_t targetWidth  = currentTarget->GetTexture()->GetWidth();
    const uint32_t targetHeight = currentTarget->GetTexture()->GetHeight();

    // ============== EARLY phase ==============
    {
        commandBuffer->BeginDebugLabel("EARLY phase");
        m_objectCullPass.PrepareEarly(commandBuffer);
        // Phase 5: cull dispatches once per (typeId) bucket. Each bucket
        // has its own [DrawBase, DrawBase+Count) slice of m_drawIndexBuffer
        // and its own IndirectCountBuffer slot (bucketIndex), so the
        // dispatches don't fight for a shared counter.
        for (size_t bi = 0; bi < buckets.size(); ++bi)
        {
            const DrawBucket& b = buckets[bi];
            m_objectCullPass.ExecuteEarly(
                commandBuffer, sceneFrameAddr, b.DrawBase, b.DrawCount, /*bucketIndex*/ static_cast<uint32_t>(bi));
        }

        commandBuffer->Barrier({
            BarrierDesc{
                .TextureView = currentTarget,
                .BeforeState = ResourceState::Undefined,
                .AfterState  = ResourceState::ColorAttachment,
            },
            BarrierDesc{
                .TextureView = m_depthView.Get(),
                .BeforeState = ResourceState::Undefined,
                .AfterState  = ResourceState::DepthWrite,
            },
        });
        {
            RenderingDesc rdesc{};
            rdesc.Width   = targetWidth;
            rdesc.Height  = targetHeight;
            rdesc.Samples = SampleCountFlagBits::Count1;
            RenderingAttachment color{};
            color.View              = currentTarget;
            color.LoadAction        = LoadOp::Clear;
            color.StoreAction       = StoreOp::Store;
            color.ClearVal.Color[0] = 0.1f;
            color.ClearVal.Color[1] = 0.1f;
            color.ClearVal.Color[2] = 0.1f;
            color.ClearVal.Color[3] = 1.0f;
            rdesc.ColorAttachments.push_back(color);
            rdesc.DepthAttachment.View                          = m_depthView.Get();
            rdesc.DepthAttachment.LoadAction                    = LoadOp::Clear;
            rdesc.DepthAttachment.StoreAction                   = StoreOp::Store;
            rdesc.DepthAttachment.ClearVal.DepthStencil.Depth   = 1.0f;
            rdesc.DepthAttachment.ClearVal.DepthStencil.Stencil = 0;
            commandBuffer->BeginRendering(rdesc);
        }
        commandBuffer->SetViewport({ 0, 0, (float)targetWidth, (float)targetHeight });
        commandBuffer->SetScissor({ 0, 0, targetWidth, targetHeight });
        for (size_t bi = 0; bi < buckets.size(); ++bi)
        {
            const DrawBucket&       b        = buckets[bi];
            const std::string       typeName = bucketTypeNames[bi];
            SpecializedPipelinePair pipelinePair =
                m_pipelineRegistry.Acquire(m_meshletDrawPass.GetPassName(), b.ShaderKey);
            m_meshletDrawPass.ExecuteEarly(commandBuffer,
                                           pipelinePair,
                                           sceneFrameAddr,
                                           b.DrawBase,
                                           b.DrawCount,
                                           b.MaterialBufferAddr,
                                           b.ShaderKey,
                                           static_cast<uint32_t>(bi));
        }
        commandBuffer->EndRendering();
        commandBuffer->EndDebugLabel();
    }

    // ============== Hi-Z pyramid build ==============
    const bool pyramidReady = m_depthPyramid.IsValid() && m_depthReducePipeline;
    if (pyramidReady)
    {
        commandBuffer->BeginDebugLabel("Hi-Z build");
        commandBuffer->Barrier(BarrierDesc{
            .TextureView = m_depthView.Get(),
            .BeforeState = ResourceState::DepthWrite,
            .AfterState  = ResourceState::ComputeShaderResource,
        });

        m_depthPyramid.Build(commandBuffer, m_depthReducePipeline.Get(), m_depthView.Get());

        commandBuffer->Barrier(BarrierDesc{
            .TextureView = m_depthView.Get(),
            .BeforeState = ResourceState::ComputeShaderResource,
            .AfterState  = ResourceState::DepthWrite,
        });
        commandBuffer->EndDebugLabel();
    }

    // ============== LATE phase ==============
    const bool runLatePass = pyramidReady && totalDrawCount > 0;
    if (runLatePass)
    {
        commandBuffer->BeginDebugLabel("LATE phase");
        commandBuffer->Barrier({
            BarrierDesc{
                .TextureView = m_depthPyramid.GetTextureView(),
                .BeforeState = ResourceState::UnorderedAccess,
                .AfterState  = ResourceState::UnorderedAccess | ResourceState::ComputeShaderResource |
                    ResourceState::TaskShaderResource | ResourceState::MeshShaderResource,
            },
            BarrierDesc{
                .TextureView = currentTarget,
                .BeforeState = ResourceState::ColorAttachment,
                .AfterState  = ResourceState::ColorAttachment,
            },
        });

        const uint32_t pyramidSlot = m_depthPyramid.GetPyramidSampledSlot();

        m_objectCullPass.PrepareLate(commandBuffer);
        for (size_t bi = 0; bi < buckets.size(); ++bi)
        {
            const DrawBucket& b = buckets[bi];
            m_objectCullPass.ExecuteLate(commandBuffer,
                                         sceneFrameAddr,
                                         b.DrawBase,
                                         b.DrawCount,
                                         pyramidSlot,
                                         /*bucketIndex*/ static_cast<uint32_t>(bi));
        }
        {
            RenderingDesc rdesc{};
            rdesc.Width   = targetWidth;
            rdesc.Height  = targetHeight;
            rdesc.Samples = SampleCountFlagBits::Count1;
            RenderingAttachment color{};
            color.View        = currentTarget;
            color.LoadAction  = LoadOp::Load;
            color.StoreAction = StoreOp::Store;
            rdesc.ColorAttachments.push_back(color);
            rdesc.DepthAttachment.View        = m_depthView.Get();
            rdesc.DepthAttachment.LoadAction  = LoadOp::Load;
            rdesc.DepthAttachment.StoreAction = StoreOp::Store;
            commandBuffer->BeginRendering(rdesc);
        }
        commandBuffer->SetViewport({ 0, 0, (float)targetWidth, (float)targetHeight });
        commandBuffer->SetScissor({ 0, 0, targetWidth, targetHeight });
        for (size_t bi = 0; bi < buckets.size(); ++bi)
        {
            const DrawBucket&       b        = buckets[bi];
            const std::string       typeName = bucketTypeNames[bi];
            SpecializedPipelinePair pipelinePair =
                m_pipelineRegistry.Acquire(m_meshletDrawPass.GetPassName(), b.ShaderKey);
            m_meshletDrawPass.ExecuteLate(commandBuffer,
                                          pipelinePair,
                                          sceneFrameAddr,
                                          b.DrawBase,
                                          b.DrawCount,
                                          pyramidSlot,
                                          b.MaterialBufferAddr,
                                          b.ShaderKey,
                                          static_cast<uint32_t>(bi));
        }

        // OutlinePass: acquire pipeline then execute
        // if (!buckets.empty())
        // {
        //     SpecializedPipelinePair outlinePair =
        //         m_pipelineRegistry.Acquire(m_outlinePass.GetPassName(), buckets[0].TypeId);
        //     m_outlinePass.Execute(commandBuffer, sceneFrameAddr, outlinePair, buckets, bucketTypeNames);
        // }

        commandBuffer->EndRendering();
        commandBuffer->EndDebugLabel();
    }

    if (transitionToPresent)
    {
        commandBuffer->Barrier(BarrierDesc{
            .TextureView = currentTarget,
            .BeforeState = ResourceState::ColorAttachment,
            .AfterState  = ResourceState::Present,
        });
    }

    commandBuffer->EndDebugLabel();
    commandBuffer->End();
    commandBuffer->Submit();
}

} // namespace Yogi
