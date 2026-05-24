#include "Renderer/ForwardRenderSystem.h"
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
    m_vertexStorageBuffer = ResourceManager::AcquireSharedResource<IBuffer>(
        BufferDesc{ MAX_VERTICES_SIZE, BufferUsage::Storage, BufferAccess::Dynamic });
    m_meshletBuffer = ResourceManager::AcquireSharedResource<IBuffer>(
        BufferDesc{ MAX_MESHLET_SIZE, BufferUsage::Storage, BufferAccess::Dynamic });
    m_meshletDataBuffer = ResourceManager::AcquireSharedResource<IBuffer>(
        BufferDesc{ MAX_MESHLET_DATA_SIZE, BufferUsage::Storage, BufferAccess::Dynamic });
    m_meshBuffer = ResourceManager::AcquireSharedResource<IBuffer>(
        BufferDesc{ MAX_MESH_SIZE, BufferUsage::Storage, BufferAccess::Dynamic });
    m_meshDrawBuffer = ResourceManager::AcquireSharedResource<IBuffer>(
        BufferDesc{ MAX_MESH_DRAW_SIZE, BufferUsage::Storage, BufferAccess::Dynamic });
    m_meshTaskIndirectBuffer = ResourceManager::AcquireSharedResource<IBuffer>(BufferDesc{
        MAX_INDIRECT_DRAW_COMMAND_SIZE, BufferUsage::Storage | BufferUsage::Indirect, BufferAccess::Dynamic });
    m_visibleDrawIndexBuffer = ResourceManager::AcquireSharedResource<IBuffer>(
        BufferDesc{ MAX_VISIBLE_DRAW_INDEX_SIZE, BufferUsage::Storage, BufferAccess::Dynamic });
    m_meshTaskIndirectCountBuffer = ResourceManager::AcquireSharedResource<IBuffer>(BufferDesc{
        MAX_INDIRECT_DRAW_COUNT_SIZE, BufferUsage::Storage | BufferUsage::Indirect, BufferAccess::Dynamic });
    m_objectVisBuffer            = ResourceManager::AcquireSharedResource<IBuffer>(
        BufferDesc{ MAX_OBJECT_VIS_SIZE, BufferUsage::Storage, BufferAccess::Dynamic });
    // Per-instance per-meshlet visibility -- one uint per (drawIndex, miLocal) slot.
    // Persistent across frames; LATE meshlet task is the sole writer.
    m_meshletVisBuffer = ResourceManager::AcquireSharedResource<IBuffer>(
        BufferDesc{ MAX_MESHLET_VIS_SIZE, BufferUsage::Storage, BufferAccess::Dynamic });
    // Per-frame static data (SceneFrame) accessed via BDA — sub-allocated each
    // frame from a single arena buffer so frame N's host writes don't race
    // frame N-1's GPU reads.
    {
        auto           swapChain  = Application::GetInstance().GetSwapChain();
        const uint32_t imageCount = swapChain->GetImageCount();
        constexpr uint64_t kSegmentSize = 4 * 1024;
        m_frameArena.Init(kSegmentSize, imageCount);
    }
    // Zero-init: first frame's EARLY skips everything (USE_PREV_VIS gate fails for all),
    // LATE then sees an empty pyramid (all depth = 1.0) and emits every visible draw.
    {
        std::vector<uint32_t> zeros(MAX_MESH_DRAWS, 0u);
        m_objectVisBuffer->UpdateData(zeros.data(), static_cast<uint32_t>(MAX_OBJECT_VIS_SIZE), 0);
    }
    // Same zero-init semantics for per-meshlet visibility: first frame, EARLY task
    // sees prev_meshlet_vis==0 for everything -> emits nothing, LATE task does the
    // full cone+frustum+Hi-Z and writes the canonical state. From frame 2 onward
    // EARLY emits whatever was visible last frame, LATE only re-tests the rest.
    //
    // 32M uints = 128 MB upload via UpdateData: this only runs at boot. UpdateData
    // is a staging-buffer copy; it does NOT block the render loop.
    {
        std::vector<uint32_t> zeros(MAX_MESHLET_VIS_COUNT, 0u);
        m_meshletVisBuffer->UpdateData(
            zeros.data(), static_cast<uint32_t>(MAX_MESHLET_VIS_SIZE), 0);
    }

    // Mesh/task SRB: SSBOs live in a SceneFrame buffer accessed via BDA; only the
    // address + per-batch DrawBase travels through push constants (16 B). The
    // descriptor at binding=0 is the Hi-Z pyramid sampler used by the LATE task
    // shader variant for meshlet occlusion. The EARLY variant doesn't reference
    // it -- a Vulkan layout that's a superset of what the SPIR-V uses is legal,
    // and lets both pipelines share one IShaderResourceBinding.
    m_shaderResourceBinding = ResourceManager::AcquireSharedResource<IShaderResourceBinding>(
        std::vector<ShaderResourceAttribute>{
            ShaderResourceAttribute{ 0, 1, ShaderResourceType::Texture, ShaderStage::Task | ShaderStage::Mesh } },
        std::vector<PushConstantRange>{
            PushConstantRange{ ShaderStage::Task | ShaderStage::Mesh, 0, static_cast<uint32_t>(sizeof(ScenePush)) } });

    // Cull SRB: shares the SceneFrame BDA via push constants; only descriptor is
    // the Hi-Z pyramid sampler at binding=0. Push constant is 32 B.
    m_cullShaderResourceBinding = ResourceManager::AcquireSharedResource<IShaderResourceBinding>(
        std::vector<ShaderResourceAttribute>{
            ShaderResourceAttribute{ 0, 1, ShaderResourceType::Texture, ShaderStage::Compute } },
        std::vector<PushConstantRange>{
            PushConstantRange{ ShaderStage::Compute, 0, static_cast<uint32_t>(sizeof(CullPush)) } });

    WRef<ShaderDesc> cullShaderEarly =
        AssetManager::AcquireAsset<ShaderDesc>("EngineAssets/Shaders/ObjectCull.comp");
    WRef<ShaderDesc> cullShaderLate =
        AssetManager::AcquireAsset<ShaderDesc>("EngineAssets/Shaders/ObjectCull.comp::LATE=1");

    PipelineDesc cullPipelineDesc{};
    cullPipelineDesc.Type                  = PipelineType::Compute;
    cullPipelineDesc.ShaderResourceBinding = m_cullShaderResourceBinding.Get();

    cullPipelineDesc.Shaders = { cullShaderEarly.Get() };
    m_cullPipelineEarly      = ResourceManager::AcquireSharedResource<IPipeline>(cullPipelineDesc);

    cullPipelineDesc.Shaders = { cullShaderLate.Get() };
    m_cullPipelineLate       = ResourceManager::AcquireSharedResource<IPipeline>(cullPipelineDesc);

    // Depth Pyramid (Hi-Z) compute pipeline.
    WRef<IShaderResourceBinding> depthReduceBindingTemplate =
        ResourceManager::AddResource(DepthPyramid::CreateReduceBindingLayout());
    WRef<ShaderDesc> depthReduceShader =
        AssetManager::AcquireAsset<ShaderDesc>("EngineAssets/Shaders/DepthReduce.comp");
    PipelineDesc depthReducePipelineDesc{};
    depthReducePipelineDesc.Type                  = PipelineType::Compute;
    depthReducePipelineDesc.Shaders               = { depthReduceShader.Get() };
    depthReducePipelineDesc.ShaderResourceBinding = depthReduceBindingTemplate.Get();
    m_depthReducePipeline = ResourceManager::AcquireSharedResource<IPipeline>(depthReducePipelineDesc);
}

ForwardRenderSystem::~ForwardRenderSystem()
{
    ResetMeshUploadCache();

    m_vertexStorageBuffer         = nullptr;
    m_meshletBuffer               = nullptr;
    m_meshletDataBuffer           = nullptr;
    m_meshDrawBuffer              = nullptr;
    m_meshTaskIndirectBuffer      = nullptr;
    m_visibleDrawIndexBuffer      = nullptr;
    m_meshTaskIndirectCountBuffer = nullptr;
    m_objectVisBuffer             = nullptr;
    m_meshletVisBuffer            = nullptr;
    m_frameArena.Shutdown();
    m_shaderResourceBinding     = nullptr;
    m_cullShaderResourceBinding = nullptr;
    m_cullPipelineEarly         = nullptr;
    m_cullPipelineLate          = nullptr;
    m_depthReducePipeline       = nullptr;
    m_depthPyramid.Reset();
    m_depthView    = nullptr;
    m_depthTexture = nullptr;
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
    // Depth Pyramid is tied to the (old) depth resolution, so force a rebuild on next frame.
    m_depthPyramid.Reset();
    return false;
}

void ForwardRenderSystem::ResetMeshUploadCache()
{
    m_meshUploadCache.Clear();
    m_cachedVertexCount     = 0;
    m_cachedMeshletCount    = 0;
    m_cachedMeshletDataSize = 0;
    // Do NOT clear m_cachedMeshMeshletCounts: it's indexed by meshIndex
    // (m_cachedMeshCount), which keeps growing across mid-frame flushes -- old
    // entries are stale but not in the way; FlushBatch only reads positions
    // [meshIndex] for live MeshDraws, all of which have valid entries appended
    // when the upload cache misses re-allocate them.
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

    // When rendering directly to a swapchain image (no camera RT target), this
    // FlushBatch is the last cmd recording on that image -- it must drop the
    // image into PRESENT_SRC before Submit() so vkQueuePresentKHR is happy.
    // Otherwise (Editor with camera.Target set) the downstream layer (ImGui)
    // is responsible for issuing its own PRESENT barrier.
    const bool transitionToPresent = !camera.Target;

    // update scene data
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

    // niagara-style symmetric frustum: derive (Lx, Lz, Ty, Tz) from the projection
    // matrix's left and top planes (rows 3+0 and 3+1 of the transpose). Right/Bottom
    // are mirrored via abs() in the cull/task shaders.
    auto    PT      = projectionMatrix.Transpose();
    Vector4 leftRaw = PT[3] + PT[0];
    Vector4 topRaw  = PT[3] + PT[1];
    Vector3 leftN   = Vector3(leftRaw.x, leftRaw.y, leftRaw.z);
    Vector3 topN    = Vector3(topRaw.x, topRaw.y, topRaw.z);
    float   leftLen = leftN.Length();
    float   topLen  = topN.Length();
    Vector4 left    = leftLen > 0.0f ? leftRaw / leftLen : leftRaw;
    Vector4 top     = topLen > 0.0f ? topRaw / topLen : topRaw;
    sceneFrame.Frustum = Vector4(left.x, -left.z, top.y, -top.z);

    sceneFrame.P00   = projectionMatrix[0][0];
    sceneFrame.P11   = projectionMatrix[1][1];
    sceneFrame.ZNear = k_zNear;
    sceneFrame.ZFar  = k_zFar;

    sceneFrame.VertexBuffer            = m_vertexStorageBuffer->GetDeviceAddress();
    sceneFrame.MeshletBuffer           = m_meshletBuffer->GetDeviceAddress();
    sceneFrame.MeshletDataBuffer       = m_meshletDataBuffer->GetDeviceAddress();
    sceneFrame.MeshDataBuffer          = m_meshBuffer->GetDeviceAddress();
    sceneFrame.MeshDrawBuffer          = m_meshDrawBuffer->GetDeviceAddress();
    sceneFrame.VisibleDrawIndexBuffer  = m_visibleDrawIndexBuffer->GetDeviceAddress();
    sceneFrame.IndirectCommandBuffer   = m_meshTaskIndirectBuffer->GetDeviceAddress();
    sceneFrame.IndirectCountBuffer     = m_meshTaskIndirectCountBuffer->GetDeviceAddress();
    sceneFrame.ObjectVisBuffer         = m_objectVisBuffer->GetDeviceAddress();
    sceneFrame.MeshletVisBuffer        = m_meshletVisBuffer->GetDeviceAddress();

    std::vector<RenderBatch>          renderBatches;
    std::unordered_map<uint64_t, int> renderBatchLookup;

    // collect and batch meshes into meshlets
    world.ViewComponents<TransformComponent, MeshRendererComponent>([&](Entity                 entity,
                                                                        TransformComponent&    meshTransform,
                                                                        MeshRendererComponent& meshRenderer) {
        auto& mesh     = meshRenderer.Mesh;
        auto& material = meshRenderer.Material;
        if (!mesh || !material)
            return;

        uint32_t meshVertexCount  = static_cast<uint32_t>(mesh->GetVertices().size());
        uint32_t meshMeshletCount = static_cast<uint32_t>(mesh->GetMeshlets().size());
        uint32_t passCount        = static_cast<uint32_t>(material->GetPasses().size());

        uint32_t totalMeshDraws = 0;
        for (const auto& batch : renderBatches)
        {
            totalMeshDraws += static_cast<uint32_t>(batch.MeshDraws.size());
        }

        if (totalMeshDraws + passCount > MAX_MESH_DRAWS)
        {
            // Mid-frame flush -- still more draws coming, don't transition to present yet.
            FlushBatch(commandBuffer, currentTarget, /*transitionToPresent=*/false,
                       sceneFrame, renderBatches, renderBatchLookup);
        }

        std::string assetKey = AssetManager::GetAssetKey(mesh);
        auto        meshKey  = MeshGPUUploadCache::BuildKey(mesh, assetKey, 0);

        uint32_t meshIndex = m_cachedMeshCount;
        if (!m_meshUploadCache.TryGet(meshKey, meshIndex))
        {
            uint32_t meshletDataSize = static_cast<uint32_t>(mesh->GetMeshletData().size());

            if (m_cachedVertexCount + meshVertexCount > MAX_VERTICES ||
                m_cachedMeshletCount + meshMeshletCount > MAX_MESHLETS ||
                m_cachedMeshletDataSize + meshletDataSize > MAX_MESHLET_DATA_SIZE / sizeof(uint32_t))
            {
                // Mid-frame flush due to upload cache pressure; not the final pass.
                FlushBatch(commandBuffer, currentTarget, /*transitionToPresent=*/false,
                           sceneFrame, renderBatches, renderBatchLookup);
                ResetMeshUploadCache();
            }

            MeshData meshData{};
            meshData.BoundingCenter  = mesh->GetCenter();
            meshData.BoundingRadius  = mesh->GetBoundingRadius();
            meshData.VertexOffset    = m_cachedVertexCount;
            meshData.MeshletOffset   = m_cachedMeshletCount;
            meshData.MeshletCount    = meshMeshletCount;
            meshData.MeshletDataBase = m_cachedMeshletDataSize;
            m_meshBuffer->UpdateData(&meshData, sizeof(MeshData), static_cast<uint64_t>(meshIndex) * sizeof(MeshData));
            if (!mesh->GetVertices().empty())
            {
                m_vertexStorageBuffer->UpdateData(mesh->GetVertices().data(),
                                                  mesh->GetVertices().size() * sizeof(VertexData),
                                                  static_cast<uint64_t>(meshData.VertexOffset) * sizeof(VertexData));
            }
            if (!mesh->GetMeshlets().empty())
            {
                m_meshletBuffer->UpdateData(mesh->GetMeshlets().data(),
                                            mesh->GetMeshlets().size() * sizeof(MeshletData),
                                            static_cast<uint64_t>(meshData.MeshletOffset) * sizeof(MeshletData));
            }
            if (!mesh->GetMeshletData().empty())
            {
                m_meshletDataBuffer->UpdateData(mesh->GetMeshletData().data(),
                                                mesh->GetMeshletData().size() * sizeof(uint32_t),
                                                static_cast<uint64_t>(meshData.MeshletDataBase) * sizeof(uint32_t));
            }

            m_cachedVertexCount += meshVertexCount;
            m_cachedMeshletCount += meshMeshletCount;
            m_cachedMeshletDataSize += meshletDataSize;
            m_cachedMeshCount += 1;

            // Parallel array indexed by meshIndex -- needed by FlushBatch's
            // MeshletVisOffset prefix sum. Every newly-allocated meshIndex
            // here gets the matching MeshletCount appended; old indices stay valid
            // (their MeshData is still resident in m_meshBuffer until the next
            // ResetMeshUploadCache, which only nukes the upload cache, not the
            // GPU-side MeshData slot).
            if (m_cachedMeshMeshletCounts.size() <= meshIndex)
            {
                m_cachedMeshMeshletCounts.resize(meshIndex + 1, 0u);
            }
            m_cachedMeshMeshletCounts[meshIndex] = meshMeshletCount;

            m_meshUploadCache.Upsert(meshKey, meshIndex);
        }

        for (auto& materialPass : material->GetPasses())
        {
            WRef<IPipeline> pipeline     = materialPass.Pipeline;
            WRef<IPipeline> latePipeline = materialPass.LatePipeline;
            if (!pipeline)
                continue;

            // Hash both pipelines so an EARLY/LATE pair distinct from another
            // pair (or the absence of a LATE variant) lands in its own batch.
            uint64_t passKey  = HashArgs(pipeline, latePipeline);
            auto     lookupIt = renderBatchLookup.find(passKey);
            if (lookupIt == renderBatchLookup.end())
            {
                renderBatchLookup[passKey] = static_cast<int>(renderBatches.size());
                RenderBatch newBatch{};
                newBatch.PipelineKey  = passKey;
                newBatch.Pipeline     = pipeline;
                newBatch.LatePipeline = latePipeline;
                renderBatches.push_back(std::move(newBatch));
                lookupIt = renderBatchLookup.find(passKey);
            }

            auto& batch = renderBatches[lookupIt->second];

            MeshDraw draw{};
            draw.Position    = meshTransform.Transform.Position;
            draw.Scale       = meshTransform.Transform.Scale;
            draw.Orientation = Vector4(meshTransform.Transform.Rotation.x,
                                       meshTransform.Transform.Rotation.y,
                                       meshTransform.Transform.Rotation.z,
                                       meshTransform.Transform.Rotation.w);
            draw.MeshIndex   = meshIndex;
            batch.MeshDraws.push_back(draw);
        }
    });

    FlushBatch(commandBuffer, currentTarget, transitionToPresent,
               sceneFrame, renderBatches, renderBatchLookup);
}

void ForwardRenderSystem::FlushBatch(ICommandBuffer*                    commandBuffer,
                                     ITextureView*                      colorView,
                                     bool                               transitionToPresent,
                                     SceneFrame&                        sceneFrame,
                                     std::vector<RenderBatch>&          renderBatches,
                                     std::unordered_map<uint64_t, int>& renderBatchLookup)
{
    commandBuffer->Wait();
    if (renderBatches.empty())
        return;

    auto           swapChain = Application::GetInstance().GetSwapChain();
    const uint32_t frameSlot = swapChain->GetCurrentFrameIndex();
    m_frameArena.BeginFrame(frameSlot);

    const uint64_t sceneFrameAddr = m_frameArena.Push(&sceneFrame, sizeof(SceneFrame));

    std::vector<MeshDraw> uploadMeshDraws;

    std::sort(renderBatches.begin(), renderBatches.end(), [](const RenderBatch& a, const RenderBatch& b) {
        return a.PipelineKey < b.PipelineKey;
    });

    for (auto& batch : renderBatches)
    {
        if (!batch.Pipeline || batch.MeshDraws.empty())
            continue;

        batch.DrawBase            = static_cast<uint32_t>(uploadMeshDraws.size());
        batch.DrawCount           = static_cast<uint32_t>(batch.MeshDraws.size());
        batch.IndirectOffsetBytes = batch.DrawBase * sizeof(uint32_t) * 3;

        uploadMeshDraws.insert(uploadMeshDraws.end(), batch.MeshDraws.begin(), batch.MeshDraws.end());
    }

    // Prefix sum: each MeshDraw's MeshletVisOffset is the cumulative
    // MeshletCount of all preceding draws. globalMeshletId in the task shader is
    // (draw.MeshletVisOffset + miLocal), so two instances of the same Cube
    // get distinct visibility slots. Capped at MAX_MESHLET_VIS_COUNT --
    // overflow ABORTS the frame: the task shader writes mvBuf via BDA which has
    // no descriptor bounds check, so an out-of-range visIdx would corrupt
    // adjacent device memory (fence/semaphore state of other queues, in the
    // worst case) and we've seen that take down the whole Vulkan device.
    {
        uint32_t cumMeshletCount = 0;
        for (auto& draw : uploadMeshDraws)
        {
            const uint32_t meshletCount = draw.MeshIndex < m_cachedMeshMeshletCounts.size()
                ? m_cachedMeshMeshletCounts[draw.MeshIndex]
                : 0u;
            draw.MeshletVisOffset = cumMeshletCount;
            cumMeshletCount += meshletCount;
        }
        if (cumMeshletCount > MAX_MESHLET_VIS_COUNT)
        {
            YG_CORE_ERROR("MeshletVis cap exceeded: needed {0}, cap {1}. Skipping frame to "
                          "avoid out-of-bounds BDA writes from the task shader.",
                          cumMeshletCount, MAX_MESHLET_VIS_COUNT);
            renderBatches.clear();
            renderBatchLookup.clear();
            return;
        }
    }

    m_meshDrawBuffer->UpdateData(
        uploadMeshDraws.data(), static_cast<uint32_t>(uploadMeshDraws.size() * sizeof(MeshDraw)), 0);

    uint32_t totalDrawCount  = static_cast<uint32_t>(uploadMeshDraws.size());
    uint32_t totalBatchCount = static_cast<uint32_t>(renderBatches.size());

    const uint32_t kLateRegionBase = static_cast<uint32_t>(MAX_MESH_DRAWS);

    const uint32_t targetWidth  = colorView->GetTexture()->GetWidth();
    const uint32_t targetHeight = colorView->GetTexture()->GetHeight();

    commandBuffer->Begin();

    // Resize + (re)bind the pyramid texture for the cull shader and the
    // mesh/task shader (LATE variant samples it for meshlet Hi-Z).
    if (m_depthPyramid.Resize(m_depthTexture->GetWidth(), m_depthTexture->GetHeight()))
    {
        m_cullShaderResourceBinding->BindTextureView(m_depthPyramid.GetTextureView(), 0, 0);
        // Same pyramid view also feeds the LATE meshlet task variant. The
        // EARLY variant's SPIR-V doesn't reference this binding, but Vulkan
        // permits a layout broader than the SPIR-V uses -- one SRB serves both.
        m_shaderResourceBinding->BindTextureView(m_depthPyramid.GetTextureView(), 0, 0);
    }
    const bool pyramidValid = m_depthPyramid.IsValid();

    // Zero the per-batch counters for BOTH EARLY (slots [0..B)) and LATE (slots [B..2B)).
    {
        std::vector<uint32_t> zeroCounts(2 * totalBatchCount, 0);
        m_meshTaskIndirectCountBuffer->UpdateData(
            zeroCounts.data(), static_cast<uint32_t>(zeroCounts.size() * sizeof(uint32_t)), 0);
    }

    if (pyramidValid)
    {
        m_depthPyramid.EnsureInitialLayout(commandBuffer);
    }

    // ================ EARLY cull dispatch ================
    commandBuffer->SetPipeline(m_cullPipelineEarly.Get());
    commandBuffer->SetShaderResourceBinding(m_cullShaderResourceBinding.Get());

    CullPush pcCull{};
    pcCull.SceneFrameAddr = sceneFrameAddr;

    for (size_t batchIndex = 0; batchIndex < renderBatches.size(); ++batchIndex)
    {
        auto& batch = renderBatches[batchIndex];
        if (!batch.Pipeline || batch.DrawCount == 0)
            continue;

        pcCull.DrawBase   = batch.DrawBase;
        pcCull.DrawCount  = batch.DrawCount;
        pcCull.OutputBase = batch.DrawBase;
        pcCull.CountIndex = static_cast<uint32_t>(batchIndex);

        commandBuffer->SetPushConstants(
            m_cullShaderResourceBinding.Get(), ShaderStage::Compute, 0, sizeof(CullPush), &pcCull);

        uint32_t dispatchX = (batch.DrawCount + CULL_WORKGROUP_SIZE - 1) / CULL_WORKGROUP_SIZE;
        commandBuffer->Dispatch(dispatchX, 1, 1);
    }

    // Indirect buffer EARLY region: compute write -> task/mesh/indirect read.
    if (totalDrawCount > 0)
    {
        BarrierDesc barrierDesc{};
        barrierDesc.Buffer      = m_meshTaskIndirectBuffer.Get();
        barrierDesc.BeforeState = ResourceState::UnorderedAccess;
        barrierDesc.AfterState =
            ResourceState::IndirectArg | ResourceState::TaskShaderResource | ResourceState::MeshShaderResource;
        commandBuffer->Barrier(barrierDesc);
    }

    // ================ EARLY rendering pass ================
    // Clears color + depth -- this is the "draws visible last frame" pass and
    // establishes the new frame's color + depth baseline. Both attachments are
    // overwritten (LoadOp::Clear), so Discard their prior contents.
    commandBuffer->Barrier(BarrierDesc{
        .TextureView = colorView,
        .BeforeState = ResourceState::Undefined,
        .AfterState  = ResourceState::ColorAttachment,
    });
    commandBuffer->Barrier(BarrierDesc{
        .TextureView = m_depthView.Get(),
        .BeforeState = ResourceState::Undefined,
        .AfterState  = ResourceState::DepthWrite,
    });
    {
        RenderingDesc rdesc{};
        rdesc.Width  = targetWidth;
        rdesc.Height = targetHeight;
        rdesc.Samples = SampleCountFlagBits::Count1;
        RenderingAttachment color{};
        color.View                   = colorView;
        color.LoadAction             = LoadOp::Clear;
        color.StoreAction            = StoreOp::Store;
        color.ClearVal.Color[0]      = 0.1f;
        color.ClearVal.Color[1]      = 0.1f;
        color.ClearVal.Color[2]      = 0.1f;
        color.ClearVal.Color[3]      = 1.0f;
        rdesc.ColorAttachments.push_back(color);
        rdesc.DepthAttachment.View                          = m_depthView.Get();
        rdesc.DepthAttachment.LoadAction                    = LoadOp::Clear;
        rdesc.DepthAttachment.StoreAction                   = StoreOp::Store;
        rdesc.DepthAttachment.ClearVal.DepthStencil.Depth   = 1.0f;
        rdesc.DepthAttachment.ClearVal.DepthStencil.Stencil = 0;
        commandBuffer->BeginRendering(rdesc);
    }
    commandBuffer->SetShaderResourceBinding(m_shaderResourceBinding.Get());
    commandBuffer->SetViewport({ 0, 0, (float)targetWidth, (float)targetHeight });
    commandBuffer->SetScissor({ 0, 0, targetWidth, targetHeight });

    for (size_t batchIndex = 0; batchIndex < renderBatches.size(); ++batchIndex)
    {
        auto& batch = renderBatches[batchIndex];
        if (!batch.Pipeline || batch.DrawCount == 0)
            continue;

        commandBuffer->SetPipeline(batch.Pipeline.Get());
        commandBuffer->SetShaderResourceBinding(m_shaderResourceBinding.Get());

        ScenePush pcScene{};
        pcScene.SceneFrameAddr = sceneFrameAddr;
        pcScene.DrawBase       = batch.DrawBase;
        pcScene._Pad0          = 0u;
        commandBuffer->SetPushConstants(
            m_shaderResourceBinding.Get(), ShaderStage::Task | ShaderStage::Mesh, 0, sizeof(ScenePush), &pcScene);
        commandBuffer->DrawMeshTasksIndirectCount(m_meshTaskIndirectBuffer.Get(),
                                                  batch.IndirectOffsetBytes,
                                                  m_meshTaskIndirectCountBuffer.Get(),
                                                  static_cast<uint32_t>(batchIndex * sizeof(uint32_t)),
                                                  batch.DrawCount,
                                                  sizeof(uint32_t) * 3);
    }

    commandBuffer->EndRendering();

    // ================ Depth Pyramid (Hi-Z) build ================
    const bool pyramidReady = m_depthPyramid.IsValid() && m_depthReducePipeline;
    if (pyramidReady)
    {
        // Depth attachment is currently in DEPTH_ATTACHMENT_OPTIMAL (set by
        // BeginRendering's transition). DepthReduce.comp samples it as a regular
        // texture, so flip to DepthRead before sampling and back after.
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
    }

    // ================ LATE cull dispatch ================
    const bool runLatePass = pyramidReady && totalDrawCount > 0;
    if (runLatePass)
    {
        commandBuffer->Barrier(BarrierDesc{
            .Buffer      = m_objectVisBuffer.Get(),
            .BeforeState = ResourceState::UnorderedAccess,
            .AfterState  = ResourceState::UnorderedAccess,
        });

        // Pyramid was just written by the reduce dispatches. Both LATE cull
        // (compute sample) and LATE meshlet task (task sample) read it; flush
        // those writes for all three shader stages while keeping the GENERAL
        // image layout (UnorderedAccess on both sides extends the dst stage
        // mask to compute+task+mesh -- see VulkanUtils::YgResourceState2VkPipelineStage2).
        commandBuffer->Barrier(BarrierDesc{
            .TextureView = m_depthPyramid.GetTextureView(),
            .BeforeState = ResourceState::UnorderedAccess,
            .AfterState  = ResourceState::UnorderedAccess,
        });

        commandBuffer->SetPipeline(m_cullPipelineLate.Get());
        commandBuffer->SetShaderResourceBinding(m_cullShaderResourceBinding.Get());

        for (size_t batchIndex = 0; batchIndex < renderBatches.size(); ++batchIndex)
        {
            auto& batch = renderBatches[batchIndex];
            if (!batch.Pipeline || batch.DrawCount == 0)
                continue;

            pcCull.DrawBase   = batch.DrawBase;
            pcCull.DrawCount  = batch.DrawCount;
            pcCull.OutputBase = kLateRegionBase + batch.DrawBase;
            pcCull.CountIndex = totalBatchCount + static_cast<uint32_t>(batchIndex);

            commandBuffer->SetPushConstants(
                m_cullShaderResourceBinding.Get(), ShaderStage::Compute, 0, sizeof(CullPush), &pcCull);

            uint32_t dispatchX = (batch.DrawCount + CULL_WORKGROUP_SIZE - 1) / CULL_WORKGROUP_SIZE;
            commandBuffer->Dispatch(dispatchX, 1, 1);
        }

        commandBuffer->Barrier(BarrierDesc{
            .Buffer      = m_meshTaskIndirectBuffer.Get(),
            .BeforeState = ResourceState::UnorderedAccess,
            .AfterState =
                ResourceState::IndirectArg | ResourceState::TaskShaderResource | ResourceState::MeshShaderResource,
        });

        // ================ LATE rendering pass ================
        // LoadOp::Load preserves both color (from EARLY) and depth (from EARLY +
        // pyramid build roundtrip) so the late draws composite on top.
        // Color: EndRendering left the image in ColorAttachmentOptimal but
        //   issued no sync to the LATE pass's attachment loads. A self-
        //   transition Barrier (ColorAttachment->ColorAttachment) carries the
        //   needed memory dependency without changing the layout.
        // Depth: the pyramid roundtrip's last barrier (ComputeShaderResource
        //   -> DepthWrite) already drained the compute writes, so no extra
        //   barrier here.
        commandBuffer->Barrier(BarrierDesc{
            .TextureView = colorView,
            .BeforeState = ResourceState::ColorAttachment,
            .AfterState  = ResourceState::ColorAttachment,
        });
        {
            RenderingDesc rdesc{};
            rdesc.Width   = targetWidth;
            rdesc.Height  = targetHeight;
            rdesc.Samples = SampleCountFlagBits::Count1;
            RenderingAttachment color{};
            color.View        = colorView;
            color.LoadAction  = LoadOp::Load;
            color.StoreAction = StoreOp::Store;
            rdesc.ColorAttachments.push_back(color);
            rdesc.DepthAttachment.View        = m_depthView.Get();
            rdesc.DepthAttachment.LoadAction  = LoadOp::Load;
            rdesc.DepthAttachment.StoreAction = StoreOp::Store;
            commandBuffer->BeginRendering(rdesc);
        }
        commandBuffer->SetShaderResourceBinding(m_shaderResourceBinding.Get());
        commandBuffer->SetViewport({ 0, 0, (float)targetWidth, (float)targetHeight });
        commandBuffer->SetScissor({ 0, 0, targetWidth, targetHeight });

        for (size_t batchIndex = 0; batchIndex < renderBatches.size(); ++batchIndex)
        {
            auto& batch = renderBatches[batchIndex];
            if (!batch.Pipeline || batch.DrawCount == 0)
                continue;

            // LATE variant when the material provides one (meshlet pipelines
            // ship a LATE-only Hi-Z shader); otherwise reuse the EARLY pipeline.
            const IPipeline* latePipeline =
                batch.LatePipeline ? batch.LatePipeline.Get() : batch.Pipeline.Get();
            commandBuffer->SetPipeline(latePipeline);
            commandBuffer->SetShaderResourceBinding(m_shaderResourceBinding.Get());

            ScenePush pcScene{};
            pcScene.SceneFrameAddr = sceneFrameAddr;
            pcScene.DrawBase       = kLateRegionBase + batch.DrawBase;
            pcScene._Pad0          = 0u;
            commandBuffer->SetPushConstants(
                m_shaderResourceBinding.Get(), ShaderStage::Task | ShaderStage::Mesh, 0, sizeof(ScenePush), &pcScene);

            const uint32_t lateIndirectOffset =
                static_cast<uint32_t>(kLateRegionBase + batch.DrawBase) * sizeof(uint32_t) * 3;
            const uint32_t lateCountOffset = static_cast<uint32_t>((totalBatchCount + batchIndex) * sizeof(uint32_t));

            commandBuffer->DrawMeshTasksIndirectCount(m_meshTaskIndirectBuffer.Get(),
                                                      lateIndirectOffset,
                                                      m_meshTaskIndirectCountBuffer.Get(),
                                                      lateCountOffset,
                                                      batch.DrawCount,
                                                      sizeof(uint32_t) * 3);
        }

        commandBuffer->EndRendering();
    }

    // Final cmd-buffer step: when the caller is rendering directly to the
    // swapchain (Sandbox, no Editor compositing layer), our last write left the
    // image in COLOR_ATTACHMENT_OPTIMAL via EndRendering. vkQueuePresentKHR
    // requires PRESENT_SRC_KHR -- with dynamic rendering there's no implicit
    // VkRenderPass finalLayout to do it for us, so we transition explicitly here.
    if (transitionToPresent)
    {
        commandBuffer->Barrier(BarrierDesc{
            .TextureView = colorView,
            .BeforeState = ResourceState::ColorAttachment,
            .AfterState  = ResourceState::Present,
        });
    }

    commandBuffer->End();
    commandBuffer->Submit();

    renderBatches.clear();
    renderBatchLookup.clear();
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
    desc.Reduction  = ITexture::SamplerReductionMode::Max;
    m_depthTexture  = ResourceManager::CreateResource<ITexture>(desc);
    m_depthView     = ResourceManager::CreateResource<ITextureView>(m_depthTexture);
}

} // namespace Yogi
