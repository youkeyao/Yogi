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

    auto swapChain = Application::GetInstance().GetSwapChain();
    m_renderPasses.push_back(ResourceManager::AcquireSharedResource<IRenderPass>(
        RenderPassDesc{ { AttachmentDesc{ swapChain->GetColorFormat(), ResourceState::Present } },
                        AttachmentDesc{ ITexture::Format::D32_FLOAT, ResourceState::DepthRead } }));

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
            ShaderResourceAttribute{ 4, 1, ShaderResourceType::StorageBuffer, ShaderStage::Compute } },
        std::vector<PushConstantRange>{
            PushConstantRange{ ShaderStage::Compute, 0, static_cast<uint32_t>(sizeof(CullData)) } });
    m_cullShaderResourceBinding->BindBuffer(m_meshBuffer.Get(), 0);
    m_cullShaderResourceBinding->BindBuffer(m_meshDrawBuffer.Get(), 1);
    m_cullShaderResourceBinding->BindBuffer(m_meshTaskIndirectBuffer.Get(), 2);
    m_cullShaderResourceBinding->BindBuffer(m_visibleDrawIndexBuffer.Get(), 3);
    m_cullShaderResourceBinding->BindBuffer(m_meshTaskIndirectCountBuffer.Get(), 4);

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
    m_shaderResourceBinding       = nullptr;
    m_cullShaderResourceBinding   = nullptr;
    m_cullPipeline                = nullptr;
    m_depthReducePipeline         = nullptr;
    m_depthPyramid.Reset();
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
    swapChain->GetCurrentCommandBuffer()->Wait();
    // Frame buffers reference the depth texture; drop them first to avoid dangling views.
    m_frameBuffers.clear();
    m_depthTexture = nullptr;
    // Depth Pyramid is tied to the (old) depth resolution, so force a rebuild on next frame.
    m_depthPyramid.Reset();
    return false;
}

void ForwardRenderSystem::UpdateDebugInput()
{
    const bool toggleKey  = Input::IsKeyPressed(YG_KEY_F9);
    const bool mipDownKey = Input::IsKeyPressed(YG_KEY_LEFT_BRACKET);
    const bool mipUpKey   = Input::IsKeyPressed(YG_KEY_RIGHT_BRACKET);

    if (toggleKey && !m_prevToggleKey)
        m_debugShowDepth = !m_debugShowDepth;
    if (mipDownKey && !m_prevMipDownKey && m_debugDepthMip > 0)
        --m_debugDepthMip;
    if (mipUpKey && !m_prevMipUpKey)
        ++m_debugDepthMip;

    m_prevToggleKey  = toggleKey;
    m_prevMipDownKey = mipDownKey;
    m_prevMipUpKey   = mipUpKey;
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

    UpdateDebugInput();

    auto            swapChain     = Application::GetInstance().GetSwapChain();
    ICommandBuffer* commandBuffer = swapChain->GetCurrentCommandBuffer();
    ITexture*       currentTarget = swapChain->GetCurrentTarget();
    if (camera.Target)
    {
        currentTarget = camera.Target.Get();
    }
    EnsureDepthTexture(currentTarget->GetWidth(), currentTarget->GetHeight());
    FrameBufferDesc desc{ currentTarget->GetWidth(),
                          currentTarget->GetHeight(),
                          m_renderPasses[0].Get(),
                          { currentTarget },
                          m_depthTexture.Get() };

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
    float   aspectRatio = (float)currentTarget->GetWidth() / (float)currentTarget->GetHeight();
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
        projectionMatrix = MathUtils::Perspective(camera.Fov, aspectRatio, 0.1f, 100.0f);
    }
    m_sceneData.ProjectionViewMatrix = projectionMatrix * viewMatrix;
    m_sceneData.ViewMatrix           = viewMatrix;
    m_sceneData.DrawBase             = 0;
    m_sceneData._Pad0                = 0;
    m_sceneData._Pad1                = 0;
    m_sceneData._Pad2                = 0;

    CullData cullData{};
    auto     PVT              = m_sceneData.ProjectionViewMatrix.Transpose();
    cullData.FrustumPlanes[0] = PVT[3] + PVT[0];
    cullData.FrustumPlanes[1] = PVT[3] - PVT[0];
    cullData.FrustumPlanes[2] = PVT[3] + PVT[1];
    cullData.FrustumPlanes[3] = PVT[3] - PVT[1];
    cullData.FrustumPlanes[4] = PVT[3] + PVT[2];
    cullData.FrustumPlanes[5] = PVT[3] - PVT[2];
    for (int i = 0; i < 6; ++i)
    {
        float length =
            Vector3(cullData.FrustumPlanes[i].x, cullData.FrustumPlanes[i].y, cullData.FrustumPlanes[i].z).Length();
        if (length > 0.0f)
            cullData.FrustumPlanes[i] /= length;
    }

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
                                     ITexture*                          blitTarget,
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

    commandBuffer->Begin();

    {
        // Reset per-batch visible draw counters before compute compaction.
        std::vector<uint32_t> zeroCounts(totalBatchCount, 0);
        m_meshTaskIndirectCountBuffer->UpdateData(
            zeroCounts.data(), static_cast<uint32_t>(zeroCounts.size() * sizeof(uint32_t)), 0);

        commandBuffer->SetPipeline(m_cullPipeline.Get());
        commandBuffer->SetShaderResourceBinding(m_cullShaderResourceBinding.Get());

        for (size_t batchIndex = 0; batchIndex < renderBatches.size(); ++batchIndex)
        {
            auto& batch = renderBatches[batchIndex];
            if (!batch.Pipeline || batch.DrawCount == 0)
                continue;

            cullData.DrawBase   = batch.DrawBase;
            cullData.DrawCount  = batch.DrawCount;
            cullData.OutputBase = batch.DrawBase;
            cullData.CountIndex = static_cast<uint32_t>(batchIndex);

            commandBuffer->SetPushConstants(
                m_cullShaderResourceBinding.Get(), ShaderStage::Compute, 0, sizeof(CullData), &cullData);

            uint32_t dispatchX = (batch.DrawCount + CULL_WORKGROUP_SIZE - 1) / CULL_WORKGROUP_SIZE;
            commandBuffer->Dispatch(dispatchX, 1, 1);
        }
        if (totalDrawCount > 0)
        {
            BarrierDesc barrierDesc{};
            barrierDesc.Buffer      = m_meshTaskIndirectBuffer.Get();
            barrierDesc.BeforeState = ResourceState::UnorderedAccess;
            barrierDesc.AfterState =
                ResourceState::IndirectArg | ResourceState::TaskShaderResource | ResourceState::MeshShaderResource;
            commandBuffer->Barrier(barrierDesc);
        }
    }

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
        drawSceneData.DrawBase  = batch.DrawBase;
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

    // ---- Depth Pyramid (Hi-Z) build -------------------------------------------------
    // Main render pass just wrote depth. Transition depth to a shader-readable state,
    // (re)build the pyramid if the target resolution changed, generate all mips, then
    // hand depth back to the graphics pipeline for future passes (the render pass'
    // initialLayout = UNDEFINED + loadOp = CLEAR still covers the next frame start, but
    // transitioning back keeps the resource in a well-defined state for any intermediate
    // reader).
    m_depthPyramid.Resize(m_depthTexture->GetWidth(), m_depthTexture->GetHeight());
    if (m_depthPyramid.IsValid() && m_depthReducePipeline)
    {
        // The render pass declared its depth attachment with FinalState = DepthRead, so on
        // exit the image is already in DEPTH_STENCIL_READ_ONLY_OPTIMAL. Transition from
        // DepthRead (not DepthWrite) to keep the validated oldLayout matching the tracked
        // layout; otherwise VUID-VkImageMemoryBarrier-oldLayout-01197 fires.
        commandBuffer->Barrier(BarrierDesc{ m_depthTexture.Get(),
                                            nullptr,
                                            ResourceState::DepthRead,
                                            ResourceState::ComputeShaderResource,
                                            0,
                                            0,
                                            0,
                                            1 });

        m_depthPyramid.Build(commandBuffer, m_depthReducePipeline.Get(), m_depthTexture.Get());

        // Make the final mip's write visible to any subsequent shader reader (e.g. a
        // future occlusion-culling compute pass). The pyramid image contract is that it
        // stays in VK_IMAGE_LAYOUT_GENERAL for its entire lifetime (see DepthPyramid.h
        // and VulkanShaderResourceBinding::BindTexture's Storage-usage special case),
        // so this is a pure memory-visibility barrier -- oldLayout == newLayout == GENERAL.
        commandBuffer->Barrier(BarrierDesc{ m_depthPyramid.GetTexture(),
                                            nullptr,
                                            ResourceState::UnorderedAccess,
                                            ResourceState::UnorderedAccess,
                                            0,
                                            0,
                                            m_depthPyramid.GetMipCount() - 1,
                                            1 });
        // Restore depth to DepthRead (matches the render pass' declared finalLayout and
        // its initialLayout for the next frame, so vkCmdBeginRenderPass' implicit
        // transition from DepthRead -> DEPTH_STENCIL_ATTACHMENT_OPTIMAL is a no-op from
        // the validator's point of view).
        commandBuffer->Barrier(BarrierDesc{ m_depthTexture.Get(),
                                            nullptr,
                                            ResourceState::ComputeShaderResource,
                                            ResourceState::DepthRead,
                                            0,
                                            0,
                                            0,
                                            1 });
    }

    // ---- Debug visualisation: copy final pyramid mip onto the render target ---------
    // The pyramid texture stays in GENERAL for its entire lifetime (see DepthPyramid.h).
    // ICommandBuffer::Blit infers the source's stable state via Usage -- for a Storage
    // texture that is FragmentShaderResource -- and its internal barriers use that state
    // as the oldLayout when transitioning to CopySource. We therefore flip the sampled
    // mip from UnorderedAccess (GENERAL) to FragmentShaderResource (SHADER_READ_ONLY)
    // before the Blit, and restore it to GENERAL afterwards so the next frame's
    // DepthPyramid::Build still sees the image in the layout its descriptors record.
    if (m_debugShowDepth && blitTarget && m_depthTexture)
    {
        const bool     usePyramid  = (m_debugDepthMip > 0) && m_depthPyramid.IsValid();
        const uint32_t pyramidMips = m_depthPyramid.IsValid() ? m_depthPyramid.GetMipCount() : 0;
        const uint32_t totalLevels = 1 + pyramidMips;
        if (m_debugDepthMip >= totalLevels)
            m_debugDepthMip = totalLevels - 1;

        const ITexture* srcTex = usePyramid ? m_depthPyramid.GetTexture() : m_depthTexture.Get();
        const uint32_t  srcMip = usePyramid ? (m_debugDepthMip - 1) : 0;

        commandBuffer->Barrier(BarrierDesc{ srcTex,
                                            nullptr,
                                            usePyramid ? ResourceState::UnorderedAccess : ResourceState::DepthRead,
                                            ResourceState::FragmentShaderResource,
                                            0,
                                            0,
                                            srcMip,
                                            1 });

        // Pyramid mip0 is allocated at previousPow2(depth) which is usually smaller than the
        // source depth (e.g. 1024x1024 for a 1920x1080 depth). DepthReduce.comp's clamp-based
        // fetch therefore only fills the top-left (depthW/2, depthH/2) region of mip0 with
        // meaningful data; the rest is edge-replicated. At mip N the valid region shrinks by
        // another factor of 2 per level. Restrict the blit source to this valid region so
        // the debug view stretches real depth content across the full render target instead
        // of dragging the clamped edge pixels into view.
        const uint32_t depthW     = m_depthTexture->GetWidth();
        const uint32_t depthH     = m_depthTexture->GetHeight();
        const uint32_t validShift = usePyramid ? (srcMip + 1) : 0;
        const uint32_t validW     = MathUtils::Max(1u, depthW >> validShift);
        const uint32_t validH     = MathUtils::Max(1u, depthH >> validShift);

        BlitDesc blit{};
        blit.SrcMipLevel = srcMip;
        blit.DstMipLevel = 0;
        blit.SrcWidth    = usePyramid ? MathUtils::Min(validW, m_depthPyramid.GetMipWidth(srcMip)) : validW;
        blit.SrcHeight   = usePyramid ? MathUtils::Min(validH, m_depthPyramid.GetMipHeight(srcMip)) : validH;
        blit.DstWidth    = blitTarget->GetWidth();
        blit.DstHeight   = blitTarget->GetHeight();
        blit.Filter      = BlitFilter::Nearest;
        commandBuffer->Blit(srcTex, blitTarget, blit);

        commandBuffer->Barrier(BarrierDesc{ srcTex,
                                            nullptr,
                                            ResourceState::FragmentShaderResource,
                                            usePyramid ? ResourceState::UnorderedAccess : ResourceState::DepthRead,
                                            0,
                                            0,
                                            srcMip,
                                            1 });
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

    m_depthTexture = ResourceManager::CreateResource<ITexture>(
        TextureDesc{ width, height, 1, m_depthFormat, ITexture::Usage::DepthStencil, SampleCountFlagBits::Count1 });
}

} // namespace Yogi
