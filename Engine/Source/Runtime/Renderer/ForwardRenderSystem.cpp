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
    m_visibilityBuffer            = ResourceManager::AcquireSharedResource<IBuffer>(
        BufferDesc{ MAX_VISIBILITY_SIZE, BufferUsage::Storage, BufferAccess::Dynamic });
    // Zero-init: first frame's EARLY skips everything (USE_PREV_VIS gate fails for all),
    // LATE then sees an empty pyramid (all depth = 1.0) and emits every visible draw.
    // From frame 2 onwards LATE's writes seed the canonical visibility for EARLY.
    {
        std::vector<uint32_t> zeros(MAX_MESH_DRAWS, 0u);
        m_visibilityBuffer->UpdateData(zeros.data(), static_cast<uint32_t>(MAX_VISIBILITY_SIZE), 0);
    }

    auto swapChain = Application::GetInstance().GetSwapChain();
    // Render pass 0: EARLY pass. Clears color + depth (same as before the Hi-Z rework).
    m_renderPasses.push_back(ResourceManager::AcquireSharedResource<IRenderPass>(
        RenderPassDesc{ { AttachmentDesc{ swapChain->GetColorFormat(), ResourceState::Present } },
                        AttachmentDesc{ ITexture::Format::D32_FLOAT, ResourceState::DepthRead } }));
    // Render pass 1: LATE pass. Preserves both color and depth written by the EARLY pass
    // plus the pyramid build's depth-read; LoadOp::Load is what makes the two half-frames
    // composite correctly into a single final image.
    m_renderPasses.push_back(ResourceManager::AcquireSharedResource<IRenderPass>(RenderPassDesc{
        { AttachmentDesc{ swapChain->GetColorFormat(), ResourceState::Present, LoadOp::Load, StoreOp::Store } },
        AttachmentDesc{ ITexture::Format::D32_FLOAT, ResourceState::DepthRead, LoadOp::Load, StoreOp::Store } }));

    m_shaderResourceBinding = ResourceManager::AcquireSharedResource<IShaderResourceBinding>(
        std::vector<ShaderResourceAttribute>{
            ShaderResourceAttribute{ 0, 1, ShaderResourceType::StorageBuffer, ShaderStage::Mesh },
            ShaderResourceAttribute{ 1, 1, ShaderResourceType::StorageBuffer, ShaderStage::Task | ShaderStage::Mesh },
            ShaderResourceAttribute{ 2, 1, ShaderResourceType::StorageBuffer, ShaderStage::Mesh },
            ShaderResourceAttribute{ 3, 1, ShaderResourceType::StorageBuffer, ShaderStage::Task | ShaderStage::Mesh },
            ShaderResourceAttribute{ 4, 1, ShaderResourceType::StorageBuffer, ShaderStage::Task | ShaderStage::Mesh },
            ShaderResourceAttribute{ 5, 1, ShaderResourceType::StorageBuffer, ShaderStage::Task } },
        std::vector<PushConstantRange>{
            PushConstantRange{ ShaderStage::Task | ShaderStage::Mesh, 0, static_cast<uint32_t>(sizeof(SceneData)) } });

    m_shaderResourceBinding->BindBuffer(m_vertexStorageBuffer.Get(), 0);
    m_shaderResourceBinding->BindBuffer(m_meshletBuffer.Get(), 1);
    m_shaderResourceBinding->BindBuffer(m_meshletDataBuffer.Get(), 2);
    m_shaderResourceBinding->BindBuffer(m_meshBuffer.Get(), 3);
    m_shaderResourceBinding->BindBuffer(m_meshDrawBuffer.Get(), 4);
    m_shaderResourceBinding->BindBuffer(m_visibleDrawIndexBuffer.Get(), 5);

    m_cullShaderResourceBinding = ResourceManager::AcquireSharedResource<IShaderResourceBinding>(
        std::vector<ShaderResourceAttribute>{
            ShaderResourceAttribute{ 0, 1, ShaderResourceType::StorageBuffer, ShaderStage::Compute },
            ShaderResourceAttribute{ 1, 1, ShaderResourceType::StorageBuffer, ShaderStage::Compute },
            ShaderResourceAttribute{ 2, 1, ShaderResourceType::StorageBuffer, ShaderStage::Compute },
            ShaderResourceAttribute{ 3, 1, ShaderResourceType::StorageBuffer, ShaderStage::Compute },
            ShaderResourceAttribute{ 4, 1, ShaderResourceType::StorageBuffer, ShaderStage::Compute },
            // 5: Hi-Z pyramid (combined image sampler; image lives in GENERAL layout).
            ShaderResourceAttribute{ 5, 1, ShaderResourceType::Texture, ShaderStage::Compute },
            // 6: single-buffered visibility[]. Read (EARLY gating) + write (LATE verdict)
            // through a compute-to-compute barrier between the two dispatches.
            ShaderResourceAttribute{ 6, 1, ShaderResourceType::StorageBuffer, ShaderStage::Compute } },
        std::vector<PushConstantRange>{
            PushConstantRange{ ShaderStage::Compute, 0, static_cast<uint32_t>(sizeof(CullData)) } });
    m_cullShaderResourceBinding->BindBuffer(m_meshBuffer.Get(), 0);
    m_cullShaderResourceBinding->BindBuffer(m_meshDrawBuffer.Get(), 1);
    m_cullShaderResourceBinding->BindBuffer(m_meshTaskIndirectBuffer.Get(), 2);
    m_cullShaderResourceBinding->BindBuffer(m_visibleDrawIndexBuffer.Get(), 3);
    m_cullShaderResourceBinding->BindBuffer(m_meshTaskIndirectCountBuffer.Get(), 4);
    // Binding 5 (pyramid) is deferred to FlushBatch() because the pyramid texture is
    // created lazily on the first Resize() -- binding a null texture trips validation.
    m_cullShaderResourceBinding->BindBuffer(m_visibilityBuffer.Get(), 6);

    WRef<ShaderDesc> cullShader = AssetManager::AcquireAsset<ShaderDesc>("EngineAssets/Shaders/ObjectCull.comp");
    PipelineDesc     cullPipelineDesc{};
    cullPipelineDesc.Type                  = PipelineType::Compute;
    cullPipelineDesc.Shaders               = { cullShader.Get() };
    cullPipelineDesc.ShaderResourceBinding = m_cullShaderResourceBinding.Get();
    m_cullPipeline                         = ResourceManager::AcquireSharedResource<IPipeline>(cullPipelineDesc);

    // Depth Pyramid (Hi-Z) compute pipeline. The binding layout used here must match the one
    // DepthPyramid creates for each mip's descriptor set -- we share a single template from
    // DepthPyramid::CreateReduceBindingLayout() so the two stay in lockstep.
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
    m_visibilityBuffer            = nullptr;
    m_shaderResourceBinding       = nullptr;
    m_cullShaderResourceBinding   = nullptr;
    m_cullPipeline                = nullptr;
    m_depthReducePipeline         = nullptr;
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
    // Frame buffers reference the depth texture; drop them first to avoid dangling views.
    m_frameBuffers.clear();
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
    FrameBufferDesc desc{
        targetTex->GetWidth(), targetTex->GetHeight(), m_renderPasses[0].Get(), { currentTarget }, m_depthView.Get()
    };

    uint64_t key = HashArgs(desc);
    auto     it  = m_frameBuffers.find(key);
    if (it == m_frameBuffers.end())
    {
        it = m_frameBuffers.insert({ key, ResourceManager::AcquireSharedResource<IFrameBuffer>(desc) }).first;
    }
    auto& frameBuffer = it->second;

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
    m_sceneData.ProjectionViewMatrix = projectionMatrix * viewMatrix;
    m_sceneData.ViewMatrix           = viewMatrix;
    m_sceneData.DrawBase             = 0;
    m_sceneData._Pad0                = 0;
    m_sceneData._Pad1                = 0;
    m_sceneData._Pad2                = 0;

    CullData cullData{};
    cullData.View = viewMatrix;

    // niagara-style symmetric frustum. Extract left/top planes from the projection
    // matrix in view space (P^T row 3 +/- row 0/1), normalize, and store only the
    // (x, z) of left and (y, z) of top -- right/bottom are mirror images about x=0
    // / y=0 in view space and reconstructed in the cull shader via abs(). Note we
    // use the projection matrix alone (not P*V) so the planes are in view space.
    auto    PT      = projectionMatrix.Transpose();
    Vector4 leftRaw = PT[3] + PT[0];
    Vector4 topRaw  = PT[3] + PT[1];
    Vector3 leftN   = Vector3(leftRaw.x, leftRaw.y, leftRaw.z);
    Vector3 topN    = Vector3(topRaw.x, topRaw.y, topRaw.z);
    float   leftLen = leftN.Length();
    float   topLen  = topN.Length();
    Vector4 left    = leftLen > 0.0f ? leftRaw / leftLen : leftRaw;
    Vector4 top     = topLen > 0.0f ? topRaw / topLen : topRaw;
    // GLM view space has -Z forward, but the cull shader negates viewPos.z so it
    // works in niagara's +Z-forward space. Negate the plane's z component to match.
    cullData.Frustum = Vector4(left.x, -left.z, top.y, -top.z);

    // P00, P11: projection diagonal entries (1/tan(fovY/2)/aspect, 1/tan(fovY/2)).
    // Read directly out of the matrix so the orthographic path stays well-defined.
    cullData.P00    = projectionMatrix[0][0];
    cullData.P11    = projectionMatrix[1][1];
    cullData.ZNear  = k_zNear;
    cullData.ZFar   = k_zFar;
    cullData.IsLate = 0u;

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
            FlushBatch(commandBuffer, frameBuffer.Get(), currentTarget, cullData, renderBatches, renderBatchLookup);
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
                FlushBatch(commandBuffer, frameBuffer.Get(), currentTarget, cullData, renderBatches, renderBatchLookup);
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

            m_meshUploadCache.Upsert(meshKey, meshIndex);
        }

        for (auto& materialPass : material->GetPasses())
        {
            WRef<IPipeline> pipeline = materialPass.Pipeline;
            if (!pipeline)
                continue;

            uint64_t passKey  = HashArgs(pipeline);
            auto     lookupIt = renderBatchLookup.find(passKey);
            if (lookupIt == renderBatchLookup.end())
            {
                renderBatchLookup[passKey] = static_cast<int>(renderBatches.size());
                RenderBatch newBatch{};
                newBatch.PipelineKey = passKey;
                newBatch.Pipeline    = pipeline;
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

    FlushBatch(commandBuffer, frameBuffer.Get(), currentTarget, cullData, renderBatches, renderBatchLookup);
}

void ForwardRenderSystem::FlushBatch(ICommandBuffer*                    commandBuffer,
                                     const IFrameBuffer*                frameBuffer,
                                     ITextureView*                      blitTarget,
                                     CullData&                          cullData,
                                     std::vector<RenderBatch>&          renderBatches,
                                     std::unordered_map<uint64_t, int>& renderBatchLookup)
{
    commandBuffer->Wait();
    if (renderBatches.empty())
        return;

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

    m_meshDrawBuffer->UpdateData(
        uploadMeshDraws.data(), static_cast<uint32_t>(uploadMeshDraws.size() * sizeof(MeshDraw)), 0);

    uint32_t totalDrawCount  = static_cast<uint32_t>(uploadMeshDraws.size());
    uint32_t totalBatchCount = static_cast<uint32_t>(renderBatches.size());

    // LATE pass offsets its indirect/visible-index writes into the second half of the
    // shared buffers so they don't stomp EARLY's output (which the main render pass is
    // still going to read via DrawMeshTasksIndirectCount).
    const uint32_t kLateRegionBase = static_cast<uint32_t>(MAX_MESH_DRAWS);

    commandBuffer->Begin();

    // -------- Resize + (re)bind the pyramid texture for the cull shader --------
    // Resize() returns true only when it (re)allocated the texture (first frame /
    // window resize), which is exactly when descriptor 5 needs to be rewritten.
    if (m_depthPyramid.Resize(m_depthTexture->GetWidth(), m_depthTexture->GetHeight()))
    {
        m_cullShaderResourceBinding->BindTextureView(m_depthPyramid.GetTextureView(), 5, 0);
    }
    const bool pyramidValid = m_depthPyramid.IsValid();

    // -------- Zero the per-batch counters for BOTH EARLY (slots [0..B)) and LATE --------
    // (slots [B..2B)). Per-frame zeroing is required because the count buffer is
    // atomicAdd'd in the cull shader.
    {
        std::vector<uint32_t> zeroCounts(2 * totalBatchCount, 0);
        m_meshTaskIndirectCountBuffer->UpdateData(
            zeroCounts.data(), static_cast<uint32_t>(zeroCounts.size() * sizeof(uint32_t)), 0);
    }

    // Pyramid descriptor records imageLayout = GENERAL (Storage-usage path in
    // VulkanShaderResourceBinding::BindTexture). On the very first frame after a
    // (re)allocation the image itself is still in UNDEFINED -- DepthPyramid::Build does
    // the Undefined->General transition internally, but Build() runs AFTER the EARLY
    // cull dispatch, so the EARLY dispatch would read a descriptor whose recorded
    // layout doesn't match the image's actual layout (VUID-vkCmdDispatch-None-09600).
    // Hoist the transition up to right before EARLY via DepthPyramid's idempotent helper;
    // Build()'s own internal call becomes a no-op thanks to the shared
    // `m_initialLayoutTransitioned` flag.
    if (pyramidValid)
    {
        m_depthPyramid.EnsureInitialLayout(commandBuffer);
    }

    // ================ EARLY cull dispatch ================
    commandBuffer->SetPipeline(m_cullPipeline.Get());
    commandBuffer->SetShaderResourceBinding(m_cullShaderResourceBinding.Get());

    for (size_t batchIndex = 0; batchIndex < renderBatches.size(); ++batchIndex)
    {
        auto& batch = renderBatches[batchIndex];
        if (!batch.Pipeline || batch.DrawCount == 0)
            continue;

        cullData.DrawBase   = batch.DrawBase;
        cullData.DrawCount  = batch.DrawCount;
        cullData.OutputBase = batch.DrawBase;                    // EARLY region
        cullData.CountIndex = static_cast<uint32_t>(batchIndex); // EARLY count slot
        cullData.IsLate     = 0u;

        commandBuffer->SetPushConstants(
            m_cullShaderResourceBinding.Get(), ShaderStage::Compute, 0, sizeof(CullData), &cullData);

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

    // ================ EARLY render pass ================
    // LoadOp::Clear on both attachments -- this is the "draws visible last frame" pass and
    // establishes the new frame's color + depth baseline.
    BeginRender(commandBuffer, frameBuffer);
    commandBuffer->SetViewport({ 0, 0, (float)frameBuffer->GetWidth(), (float)frameBuffer->GetHeight() });
    commandBuffer->SetScissor({ 0, 0, frameBuffer->GetWidth(), frameBuffer->GetHeight() });

    for (size_t batchIndex = 0; batchIndex < renderBatches.size(); ++batchIndex)
    {
        auto& batch = renderBatches[batchIndex];
        if (!batch.Pipeline || batch.DrawCount == 0)
            continue;

        commandBuffer->SetPipeline(batch.Pipeline.Get());
        commandBuffer->SetShaderResourceBinding(m_shaderResourceBinding.Get());

        SceneData drawSceneData = m_sceneData;
        drawSceneData.DrawBase  = batch.DrawBase; // reads visibleDrawIndices[batch.DrawBase + i]
        commandBuffer->SetPushConstants(
            m_shaderResourceBinding.Get(), ShaderStage::Task | ShaderStage::Mesh, 0, sizeof(SceneData), &drawSceneData);
        commandBuffer->DrawMeshTasksIndirectCount(m_meshTaskIndirectBuffer.Get(),
                                                  batch.IndirectOffsetBytes,
                                                  m_meshTaskIndirectCountBuffer.Get(),
                                                  static_cast<uint32_t>(batchIndex * sizeof(uint32_t)),
                                                  batch.DrawCount,
                                                  sizeof(uint32_t) * 3);
    }

    EndRender(commandBuffer);

    // ================ Depth Pyramid (Hi-Z) build ================
    // After EARLY render we have a depth buffer representing everything we know was
    // visible last frame + passed frustum. Build the pyramid from that -- it's the
    // "conservative occluder set" the LATE pass will query. Resize / (re)bind already
    // happened at the top of FlushBatch (the depth texture doesn't resize mid-frame).
    const bool pyramidReady = m_depthPyramid.IsValid() && m_depthReducePipeline;
    if (pyramidReady)
    {
        // Same dance as before: DepthRead (render pass finalLayout) -> ComputeShaderResource
        // for sampling in DepthReduce.comp, then back to DepthRead.
        commandBuffer->Barrier(BarrierDesc{
            .TextureView = m_depthView.Get(),
            .BeforeState = ResourceState::DepthRead,
            .AfterState  = ResourceState::ComputeShaderResource,
        });

        m_depthPyramid.Build(commandBuffer, m_depthReducePipeline.Get(), m_depthView.Get());

        commandBuffer->Barrier(BarrierDesc{
            .TextureView = m_depthView.Get(),
            .BeforeState = ResourceState::ComputeShaderResource,
            .AfterState  = ResourceState::DepthRead,
        });
    }

    // ================ LATE cull dispatch ================
    // Re-test every draw against the freshly-built pyramid; the shader skips any draw
    // that EARLY already marked visible (prevVisible[i] != 0), so this only adds the
    // draws that EARLY rejected but the new pyramid now reveals as visible.
    const bool runLatePass = pyramidReady && totalDrawCount > 0;
    if (runLatePass)
    {
        // Make EARLY's visibility[] writes visible to LATE's reads. Both map to the
        // same VkBuffer; a compute->compute memory barrier is what makes the single-
        // buffered aliasing of bindings 6 and 7 safe.
        commandBuffer->Barrier(BarrierDesc{
            .Buffer      = m_visibilityBuffer.Get(),
            .BeforeState = ResourceState::UnorderedAccess,
            .AfterState  = ResourceState::UnorderedAccess,
        });

        commandBuffer->SetPipeline(m_cullPipeline.Get());
        commandBuffer->SetShaderResourceBinding(m_cullShaderResourceBinding.Get());

        for (size_t batchIndex = 0; batchIndex < renderBatches.size(); ++batchIndex)
        {
            auto& batch = renderBatches[batchIndex];
            if (!batch.Pipeline || batch.DrawCount == 0)
                continue;

            cullData.DrawBase   = batch.DrawBase;
            cullData.DrawCount  = batch.DrawCount;
            cullData.OutputBase = kLateRegionBase + batch.DrawBase;                    // LATE region
            cullData.CountIndex = totalBatchCount + static_cast<uint32_t>(batchIndex); // LATE count slot
            cullData.IsLate     = 1u;

            commandBuffer->SetPushConstants(
                m_cullShaderResourceBinding.Get(), ShaderStage::Compute, 0, sizeof(CullData), &cullData);

            uint32_t dispatchX = (batch.DrawCount + CULL_WORKGROUP_SIZE - 1) / CULL_WORKGROUP_SIZE;
            commandBuffer->Dispatch(dispatchX, 1, 1);
        }

        // LATE indirect region visible to next draw pass.
        commandBuffer->Barrier(BarrierDesc{
            .Buffer      = m_meshTaskIndirectBuffer.Get(),
            .BeforeState = ResourceState::UnorderedAccess,
            .AfterState =
                ResourceState::IndirectArg | ResourceState::TaskShaderResource | ResourceState::MeshShaderResource,
        });

        // ================ LATE render pass ================
        // LoadOp::Load preserves both color (from EARLY) and depth (from EARLY + pyramid
        // build roundtrip) so the late draws composite on top.
        commandBuffer->BeginRenderPass(m_renderPasses[1].Get(),
                                       frameBuffer,
                                       { ClearValue{ 0.0f, 0.0f, 0.0f, 0.0f } }, // ignored -- LoadOp::Load
                                       ClearValue{ 1.0f, 0 });
        commandBuffer->SetShaderResourceBinding(m_shaderResourceBinding.Get());
        commandBuffer->SetViewport({ 0, 0, (float)frameBuffer->GetWidth(), (float)frameBuffer->GetHeight() });
        commandBuffer->SetScissor({ 0, 0, frameBuffer->GetWidth(), frameBuffer->GetHeight() });

        for (size_t batchIndex = 0; batchIndex < renderBatches.size(); ++batchIndex)
        {
            auto& batch = renderBatches[batchIndex];
            if (!batch.Pipeline || batch.DrawCount == 0)
                continue;

            commandBuffer->SetPipeline(batch.Pipeline.Get());
            commandBuffer->SetShaderResourceBinding(m_shaderResourceBinding.Get());

            // The main rendering shader reads visibleDrawIndices[drawSceneData.DrawBase + i],
            // which must point at the LATE region.
            SceneData drawSceneData = m_sceneData;
            drawSceneData.DrawBase  = kLateRegionBase + batch.DrawBase;
            commandBuffer->SetPushConstants(m_shaderResourceBinding.Get(),
                                            ShaderStage::Task | ShaderStage::Mesh,
                                            0,
                                            sizeof(SceneData),
                                            &drawSceneData);

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

        commandBuffer->EndRenderPass();
    }

    commandBuffer->End();
    commandBuffer->Submit();

    renderBatches.clear();
    renderBatchLookup.clear();
}

void ForwardRenderSystem::BeginRender(ICommandBuffer* commandBuffer, const IFrameBuffer* frameBuffer)
{
    commandBuffer->BeginRenderPass(
        m_renderPasses[0].Get(), frameBuffer, { ClearValue{ 0.1f, 0.1f, 0.1f, 1.0f } }, ClearValue{ 1.0f, 0 });
    commandBuffer->SetShaderResourceBinding(m_shaderResourceBinding.Get());
}

void ForwardRenderSystem::EndRender(ICommandBuffer* commandBuffer)
{
    commandBuffer->EndRenderPass();
}

void ForwardRenderSystem::EnsureDepthTexture(uint32_t width, uint32_t height)
{
    if (m_depthTexture && m_depthTexture->GetWidth() == width && m_depthTexture->GetHeight() == height)
        return;

    // Reduction::Max: DepthReduce.comp samples this texture for the source -> pyramid-mip-0
    // pass and relies on the sampler doing the 2x2 max in hardware.
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
