#include "Renderer/ForwardRenderSystem.h"
#include "Resources/AssetManager/AssetManager.h"
#include "Resources/ResourceManager/ResourceManager.h"
#include "Core/Application.h"
#include "Scene/World.h"
#include "Math/Vector.h"

namespace Yogi
{

namespace
{
struct DepthPyramidPushConstant
{
    uint32_t SourceWidth;
    uint32_t SourceHeight;
    uint32_t SourceTexelStride;
    uint32_t UseMinReduction;
};

uint32_t CalcDepthPyramidMips(uint32_t width, uint32_t height)
{
    uint32_t longest = std::max(width, height);
    uint32_t levels  = 1;
    while (longest > 1)
    {
        longest >>= 1;
        ++levels;
    }
    return levels;
}
} // namespace

ForwardRenderSystem::ForwardRenderSystem()
{
    m_vertexStorageBuffer = ResourceManager::GetSharedResource<IBuffer>(
        BufferDesc{ MAX_VERTICES_SIZE, BufferUsage::Storage, BufferAccess::Dynamic });
    m_meshletBuffer = ResourceManager::GetSharedResource<IBuffer>(
        BufferDesc{ MAX_MESHLET_SIZE, BufferUsage::Storage, BufferAccess::Dynamic });
    m_meshletDataBuffer = ResourceManager::GetSharedResource<IBuffer>(
        BufferDesc{ MAX_MESHLET_DATA_SIZE, BufferUsage::Storage, BufferAccess::Dynamic });
    m_meshBuffer = ResourceManager::GetSharedResource<IBuffer>(
        BufferDesc{ MAX_MESH_SIZE, BufferUsage::Storage, BufferAccess::Dynamic });
    m_meshDrawBuffer = ResourceManager::GetSharedResource<IBuffer>(
        BufferDesc{ MAX_MESH_DRAW_SIZE, BufferUsage::Storage, BufferAccess::Dynamic });
    m_meshTaskIndirectBuffer = ResourceManager::GetSharedResource<IBuffer>(BufferDesc{
        MAX_INDIRECT_DRAW_COMMAND_SIZE, BufferUsage::Storage | BufferUsage::Indirect, BufferAccess::Dynamic });
    m_visibleDrawIndexBuffer = ResourceManager::GetSharedResource<IBuffer>(
        BufferDesc{ MAX_VISIBLE_DRAW_INDEX_SIZE, BufferUsage::Storage, BufferAccess::Dynamic });
    m_meshTaskIndirectCountBuffer = ResourceManager::GetSharedResource<IBuffer>(BufferDesc{
        MAX_INDIRECT_DRAW_COUNT_SIZE, BufferUsage::Storage | BufferUsage::Indirect, BufferAccess::Dynamic });

    auto& swapChain = Application::GetInstance().GetSwapChain();
    m_depthFormat   = ITexture::Format::D32_FLOAT;
    m_numSamples    = swapChain->GetNumSamples();
    m_renderPasses.push_back(Yogi::ResourceManager::GetSharedResource<Yogi::IRenderPass>(
        Yogi::RenderPassDesc{ { Yogi::AttachmentDesc{ swapChain->GetColorFormat(), Yogi::ResourceState::Present } },
                              Yogi::AttachmentDesc{ m_depthFormat, Yogi::ResourceState::DepthRead },
                              m_numSamples }));

    m_shaderResourceBinding = ResourceManager::GetSharedResource<IShaderResourceBinding>(
        std::vector<ShaderResourceAttribute>{
            ShaderResourceAttribute{ 0, 1, ShaderResourceType::StorageBuffer, ShaderStage::Mesh },
            ShaderResourceAttribute{ 1, 1, ShaderResourceType::StorageBuffer, ShaderStage::Task | ShaderStage::Mesh },
            ShaderResourceAttribute{ 2, 1, ShaderResourceType::StorageBuffer, ShaderStage::Mesh },
            ShaderResourceAttribute{ 3, 1, ShaderResourceType::StorageBuffer, ShaderStage::Task | ShaderStage::Mesh },
            ShaderResourceAttribute{ 4, 1, ShaderResourceType::StorageBuffer, ShaderStage::Task | ShaderStage::Mesh },
            ShaderResourceAttribute{ 5, 1, ShaderResourceType::StorageBuffer, ShaderStage::Task } },
        std::vector<PushConstantRange>{
            PushConstantRange{ ShaderStage::Task | ShaderStage::Mesh, 0, static_cast<uint32_t>(sizeof(SceneData)) } });

    m_shaderResourceBinding->BindBuffer(View<IBuffer>::Create(m_vertexStorageBuffer), 0);
    m_shaderResourceBinding->BindBuffer(View<IBuffer>::Create(m_meshletBuffer), 1);
    m_shaderResourceBinding->BindBuffer(View<IBuffer>::Create(m_meshletDataBuffer), 2);
    m_shaderResourceBinding->BindBuffer(View<IBuffer>::Create(m_meshBuffer), 3);
    m_shaderResourceBinding->BindBuffer(View<IBuffer>::Create(m_meshDrawBuffer), 4);
    m_shaderResourceBinding->BindBuffer(View<IBuffer>::Create(m_visibleDrawIndexBuffer), 5);

    m_cullShaderResourceBinding = ResourceManager::GetSharedResource<IShaderResourceBinding>(
        std::vector<ShaderResourceAttribute>{
            ShaderResourceAttribute{ 0, 1, ShaderResourceType::StorageBuffer, ShaderStage::Compute },
            ShaderResourceAttribute{ 1, 1, ShaderResourceType::StorageBuffer, ShaderStage::Compute },
            ShaderResourceAttribute{ 2, 1, ShaderResourceType::StorageBuffer, ShaderStage::Compute },
            ShaderResourceAttribute{ 3, 1, ShaderResourceType::StorageBuffer, ShaderStage::Compute },
            ShaderResourceAttribute{ 4, 1, ShaderResourceType::StorageBuffer, ShaderStage::Compute } },
        std::vector<PushConstantRange>{
            PushConstantRange{ ShaderStage::Compute, 0, static_cast<uint32_t>(sizeof(CullData)) } });
    m_cullShaderResourceBinding->BindBuffer(View<IBuffer>::Create(m_meshBuffer), 0);
    m_cullShaderResourceBinding->BindBuffer(View<IBuffer>::Create(m_meshDrawBuffer), 1);
    m_cullShaderResourceBinding->BindBuffer(View<IBuffer>::Create(m_meshTaskIndirectBuffer), 2);
    m_cullShaderResourceBinding->BindBuffer(View<IBuffer>::Create(m_visibleDrawIndexBuffer), 3);
    m_cullShaderResourceBinding->BindBuffer(View<IBuffer>::Create(m_meshTaskIndirectCountBuffer), 4);

    WRef<ShaderDesc> cullShader = AssetManager::GetAsset<ShaderDesc>("EngineAssets/Shaders/ObjectCull.comp");
    PipelineDesc     cullPipelineDesc{};
    cullPipelineDesc.Type                  = PipelineType::Compute;
    cullPipelineDesc.Shaders               = { View<ShaderDesc>::Create(cullShader) };
    cullPipelineDesc.ShaderResourceBinding = View<IShaderResourceBinding>::Create(m_cullShaderResourceBinding);
    m_cullPipeline                         = ResourceManager::GetSharedResource<IPipeline>(cullPipelineDesc);

    m_depthPyramidMipBinding = IShaderResourceBinding::Create(
        std::vector<ShaderResourceAttribute>{
            ShaderResourceAttribute{ 0, 1, ShaderResourceType::Texture, ShaderStage::Compute },
            ShaderResourceAttribute{ 1, 1, ShaderResourceType::StorageImage, ShaderStage::Compute } },
        std::vector<PushConstantRange>{
            PushConstantRange{ ShaderStage::Compute, 0, static_cast<uint32_t>(sizeof(DepthPyramidPushConstant)) } });

    PipelineDesc depthPyramidPipelineDesc{};
    depthPyramidPipelineDesc.Type = PipelineType::Compute;
    WRef<ShaderDesc> depthPyramidFirstMipShader =
        AssetManager::GetAsset<ShaderDesc>("EngineAssets/Shaders/DepthPyramidFirstMip.comp");
    depthPyramidPipelineDesc.Shaders = { View<ShaderDesc>::Create(depthPyramidFirstMipShader) };
    depthPyramidPipelineDesc.ShaderResourceBinding =
        View<IShaderResourceBinding>::Create(WRef<IShaderResourceBinding>::Create(m_depthPyramidMipBinding));
    m_depthPyramidFirstMipPipeline = ResourceManager::GetSharedResource<IPipeline>(depthPyramidPipelineDesc);

    depthPyramidPipelineDesc.Type       = PipelineType::Compute;
    WRef<ShaderDesc> depthPyramidShader = AssetManager::GetAsset<ShaderDesc>("EngineAssets/Shaders/DepthPyramid.comp");
    depthPyramidPipelineDesc.Shaders    = { View<ShaderDesc>::Create(depthPyramidShader) };
    depthPyramidPipelineDesc.ShaderResourceBinding =
        View<IShaderResourceBinding>::Create(WRef<IShaderResourceBinding>::Create(m_depthPyramidMipBinding));
    m_depthPyramidPipeline = ResourceManager::GetSharedResource<IPipeline>(depthPyramidPipelineDesc);
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
    m_depthPyramidMipBindings.clear();
    m_depthPyramidFirstMipBindingCache.clear();
    m_cullPipeline                 = nullptr;
    m_depthPyramidFirstMipPipeline = nullptr;
    m_depthPyramidPipeline         = nullptr;
    m_depthPyramidTexture          = nullptr;
    m_depthTexture                 = nullptr;
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
    auto& swapChain = Application::GetInstance().GetSwapChain();
    swapChain->GetCurrentCommandBuffer()->Wait();
    m_frameBuffers.clear();
    m_depthTexture = nullptr;
    m_depthPyramidTexture = nullptr;
    m_depthPyramidMipBindings.clear();
    m_depthPyramidFirstMipBindingCache.clear();
    m_depthPyramidValid  = false;
    m_depthPyramidWidth  = 0;
    m_depthPyramidHeight = 0;
    m_depthPyramidMips   = 1;
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

    auto&                swapChain     = Application::GetInstance().GetSwapChain();
    WRef<ICommandBuffer> commandBuffer = swapChain->GetCurrentCommandBuffer();
    WRef<ITexture>       currentTarget = swapChain->GetCurrentTarget();
    if (camera.Target)
    {
        currentTarget = camera.Target;
    }
    EnsureDepthTexture(currentTarget->GetWidth(), currentTarget->GetHeight());
    auto            currentDepth = m_depthTexture;
    FrameBufferDesc desc{ currentTarget->GetWidth(),
                          currentTarget->GetHeight(),
                          View<IRenderPass>::Create(m_renderPasses[0]),
                          { View<ITexture>::Create(currentTarget) },
                          View<ITexture>::Create(currentDepth) };
    uint64_t        key = HashArgs(desc);
    auto            it  = m_frameBuffers.find(key);
    if (it == m_frameBuffers.end())
    {
        it = m_frameBuffers.insert({ key, ResourceManager::GetSharedResource<IFrameBuffer>(desc) }).first;
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

    struct RenderBatch
    {
        uint64_t              PipelineKey = 0;
        WRef<IPipeline>       Pipeline;
        std::vector<MeshDraw> MeshDraws;
        uint32_t              DrawBase            = 0;
        uint32_t              DrawCount           = 0;
        uint32_t              IndirectOffsetBytes = 0;
    };

    std::vector<RenderBatch>          renderBatches;
    std::unordered_map<uint64_t, int> renderBatchLookup;

    auto flushBatch = [&]() {
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

            commandBuffer->SetPipeline(View<IPipeline>::Create(m_cullPipeline));
            commandBuffer->SetShaderResourceBinding(View<IShaderResourceBinding>::Create(m_cullShaderResourceBinding));

            for (size_t batchIndex = 0; batchIndex < renderBatches.size(); ++batchIndex)
            {
                auto& batch = renderBatches[batchIndex];
                if (!batch.Pipeline || batch.DrawCount == 0)
                    continue;

                cullData.DrawBase   = batch.DrawBase;
                cullData.DrawCount  = batch.DrawCount;
                cullData.OutputBase = batch.DrawBase;
                cullData.CountIndex = static_cast<uint32_t>(batchIndex);

                commandBuffer->SetPushConstants(View<IShaderResourceBinding>::Create(m_cullShaderResourceBinding),
                                                ShaderStage::Compute,
                                                0,
                                                sizeof(CullData),
                                                &cullData);

                uint32_t dispatchX = (batch.DrawCount + CULL_WORKGROUP_SIZE - 1) / CULL_WORKGROUP_SIZE;
                commandBuffer->Dispatch(dispatchX, 1, 1);
            }
            if (totalDrawCount > 0)
            {
                BarrierDesc barrierDesc{};
                barrierDesc.Buffer      = View<IBuffer>::Create(m_meshTaskIndirectBuffer);
                barrierDesc.BeforeState = ResourceState::UnorderedAccess;
                barrierDesc.AfterState =
                    ResourceState::IndirectArg | ResourceState::TaskShaderResource | ResourceState::MeshShaderResource;
                commandBuffer->Barrier(barrierDesc);
            }
        }

        BeginRender(View<ICommandBuffer>::Create(commandBuffer), View<IFrameBuffer>::Create(frameBuffer));
        commandBuffer->SetViewport({ 0, 0, (float)frameBuffer->GetWidth(), (float)frameBuffer->GetHeight() });
        commandBuffer->SetScissor({ 0, 0, frameBuffer->GetWidth(), frameBuffer->GetHeight() });

        for (size_t batchIndex = 0; batchIndex < renderBatches.size(); ++batchIndex)
        {
            auto& batch = renderBatches[batchIndex];
            if (!batch.Pipeline || batch.DrawCount == 0)
                continue;

            commandBuffer->SetPipeline(View<IPipeline>::Create(batch.Pipeline));
            commandBuffer->SetShaderResourceBinding(View<IShaderResourceBinding>::Create(m_shaderResourceBinding));

            SceneData drawSceneData = m_sceneData;
            drawSceneData.DrawBase  = batch.DrawBase;
            commandBuffer->SetPushConstants(View<IShaderResourceBinding>::Create(m_shaderResourceBinding),
                                            ShaderStage::Task | ShaderStage::Mesh,
                                            0,
                                            sizeof(SceneData),
                                            &drawSceneData);
            commandBuffer->DrawMeshTasksIndirectCount(View<IBuffer>::Create(m_meshTaskIndirectBuffer),
                                                      batch.IndirectOffsetBytes,
                                                      View<IBuffer>::Create(m_meshTaskIndirectCountBuffer),
                                                      static_cast<uint32_t>(batchIndex * sizeof(uint32_t)),
                                                      batch.DrawCount,
                                                      sizeof(uint32_t) * 3);
        }

        EndRender(View<ICommandBuffer>::Create(commandBuffer));
        BuildDepthPyramid(View<ICommandBuffer>::Create(commandBuffer), View<ITexture>::Create(currentDepth));
        if (m_depthPyramidTexture && m_depthPyramidValid && m_depthPyramidTexture->GetMipLevels() > 1)
        {
            BlitDesc blitDesc{};
            blitDesc.SrcMipLevel = 6;
            commandBuffer->Blit(
                View<ITexture>::Create(m_depthPyramidTexture), View<ITexture>::Create(currentTarget), blitDesc);
        }
        commandBuffer->End();
        commandBuffer->Submit();

        renderBatches.clear();
        renderBatchLookup.clear();
    };

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
            flushBatch();
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
                flushBatch();
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

    flushBatch();
}

void ForwardRenderSystem::BeginRender(View<ICommandBuffer> commandBuffer, View<IFrameBuffer> frameBuffer)
{
    auto* mutableCommandBuffer = const_cast<ICommandBuffer*>(commandBuffer.Get());
    mutableCommandBuffer->BeginRenderPass(View<IRenderPass>::Create(m_renderPasses[0]),
                                          frameBuffer,
                                          { ClearValue{ 0.1f, 0.1f, 0.1f, 1.0f } },
                                          ClearValue{ 1.0f, 0 });
    mutableCommandBuffer->SetShaderResourceBinding(View<IShaderResourceBinding>::Create(m_shaderResourceBinding));
}

void ForwardRenderSystem::EndRender(View<ICommandBuffer> commandBuffer)
{
    const_cast<ICommandBuffer*>(commandBuffer.Get())->EndRenderPass();
}

void ForwardRenderSystem::EnsureDepthTexture(uint32_t width, uint32_t height)
{
    if (m_depthTexture && m_depthTexture->GetWidth() == width && m_depthTexture->GetHeight() == height)
        return;

    m_depthTexture = ResourceManager::CreateResource<ITexture>(TextureDesc{
        width, height, 1, m_depthFormat, ITexture::Usage::DepthStencil, m_numSamples });
}

void ForwardRenderSystem::EnsureDepthPyramidResources(View<ICommandBuffer> commandBuffer,
                                                      uint32_t             width,
                                                      uint32_t             height,
                                                      View<ITexture>       depthTexture)
{
    auto* mutableCommandBuffer = const_cast<ICommandBuffer*>(commandBuffer.Get());
    if (width == 0 || height == 0)
        return;

    uint32_t mipCount = CalcDepthPyramidMips(width, height);
    if (m_depthPyramidTexture && m_depthPyramidWidth == width && m_depthPyramidHeight == height &&
        m_depthPyramidMips == mipCount)
    {
        return;
    }

    m_depthPyramidTexture = ResourceManager::CreateResource<ITexture>(TextureDesc{
        width, height, mipCount, ITexture::Format::R32_FLOAT, ITexture::Usage::Storage, SampleCountFlagBits::Count1 });
    m_depthPyramidWidth   = width;
    m_depthPyramidHeight  = height;
    m_depthPyramidMips    = mipCount;
    m_depthPyramidValid   = false;

    m_depthPyramidMipBindings.clear();
    m_depthPyramidFirstMipBindingCache.clear();
    for (uint32_t mip = 0; mip < m_depthPyramidMips; ++mip)
    {
        auto mipBinding = IShaderResourceBinding::Create(
            std::vector<ShaderResourceAttribute>{
                ShaderResourceAttribute{ 0, 1, ShaderResourceType::Texture, ShaderStage::Compute },
                ShaderResourceAttribute{ 1, 1, ShaderResourceType::StorageImage, ShaderStage::Compute } },
            std::vector<PushConstantRange>{ PushConstantRange{
                ShaderStage::Compute, 0, static_cast<uint32_t>(sizeof(DepthPyramidPushConstant)) } });

        if (mip == 0)
        {
            mipBinding->BindTexture(depthTexture, 0);
        }
        else
        {
            mipBinding->BindTexture(View<ITexture>::Create(m_depthPyramidTexture), 0, 0, mip - 1);
        }
        mipBinding->BindTexture(View<ITexture>::Create(m_depthPyramidTexture), 1, 0, mip);

        m_depthPyramidMipBindings.push_back(std::move(mipBinding));
    }

    mutableCommandBuffer->Barrier(BarrierDesc{ View<ITexture>::Create(m_depthPyramidTexture),
                                               nullptr,
                                               ResourceState::None,
                                               ResourceState::ComputeShaderResource,
                                               0,
                                               0,
                                               0,
                                               0 });
}

void ForwardRenderSystem::BuildDepthPyramid(View<ICommandBuffer> commandBuffer, View<ITexture> depthTexture)
{
    if (!commandBuffer || !depthTexture || !m_depthPyramidFirstMipPipeline || !m_depthPyramidPipeline)
        return;

    auto* mutableCommandBuffer = const_cast<ICommandBuffer*>(commandBuffer.Get());

    EnsureDepthPyramidResources(commandBuffer, depthTexture->GetWidth(), depthTexture->GetHeight(), depthTexture);
    if (!m_depthPyramidTexture)
        return;

    mutableCommandBuffer->Barrier(BarrierDesc{
        depthTexture, nullptr, ResourceState::DepthRead, ResourceState::ComputeShaderResource, 0, 0, 0, 1 });

    for (uint32_t mip = 0; mip < m_depthPyramidMips; ++mip)
    {
        mutableCommandBuffer->SetPipeline(
            View<IPipeline>::Create(mip == 0 ? m_depthPyramidFirstMipPipeline : m_depthPyramidPipeline));

        WRef<IShaderResourceBinding> mipBinding;
        if (mip == 0)
        {
            uint64_t depthKey = reinterpret_cast<uint64_t>(depthTexture.Get());
            auto     it       = m_depthPyramidFirstMipBindingCache.find(depthKey);
            if (it == m_depthPyramidFirstMipBindingCache.end())
            {
                auto firstMipBinding = IShaderResourceBinding::Create(
                    std::vector<ShaderResourceAttribute>{
                        ShaderResourceAttribute{ 0, 1, ShaderResourceType::Texture, ShaderStage::Compute },
                        ShaderResourceAttribute{ 1, 1, ShaderResourceType::StorageImage, ShaderStage::Compute } },
                    std::vector<PushConstantRange>{ PushConstantRange{
                        ShaderStage::Compute, 0, static_cast<uint32_t>(sizeof(DepthPyramidPushConstant)) } });
                firstMipBinding->BindTexture(depthTexture, 0);
                firstMipBinding->BindTexture(View<ITexture>::Create(m_depthPyramidTexture), 1, 0, 0);
                it = m_depthPyramidFirstMipBindingCache.emplace(depthKey, std::move(firstMipBinding)).first;
            }
            mipBinding = WRef<IShaderResourceBinding>::Create(it->second);
        }
        else
        {
            mipBinding = WRef<IShaderResourceBinding>::Create(m_depthPyramidMipBindings[mip]);
        }

        mutableCommandBuffer->Barrier(BarrierDesc{ View<ITexture>::Create(m_depthPyramidTexture),
                                                   nullptr,
                                                   ResourceState::ComputeShaderResource,
                                                   ResourceState::UnorderedAccess,
                                                   0,
                                                   0,
                                                   mip,
                                                   1 });

        mutableCommandBuffer->SetShaderResourceBinding(View<IShaderResourceBinding>::Create(mipBinding));

        uint32_t dstWidth  = std::max(1u, m_depthPyramidWidth >> mip);
        uint32_t dstHeight = std::max(1u, m_depthPyramidHeight >> mip);

        DepthPyramidPushConstant push{};
        if (mip == 0)
        {
            push.SourceWidth       = m_depthPyramidWidth;
            push.SourceHeight      = m_depthPyramidHeight;
            push.SourceTexelStride = 1;
        }
        else
        {
            push.SourceWidth       = std::max(1u, m_depthPyramidWidth >> (mip - 1));
            push.SourceHeight      = std::max(1u, m_depthPyramidHeight >> (mip - 1));
            push.SourceTexelStride = 2;
        }
        push.UseMinReduction = 1;

        mutableCommandBuffer->SetPushConstants(View<IShaderResourceBinding>::Create(mipBinding),
                                               ShaderStage::Compute,
                                               0,
                                               sizeof(DepthPyramidPushConstant),
                                               &push);

        constexpr uint32_t GROUP_SIZE = 8;
        uint32_t           dispatchX  = (dstWidth + GROUP_SIZE - 1) / GROUP_SIZE;
        uint32_t           dispatchY  = (dstHeight + GROUP_SIZE - 1) / GROUP_SIZE;
        mutableCommandBuffer->Dispatch(dispatchX, dispatchY, 1);

        mutableCommandBuffer->Barrier(BarrierDesc{ View<ITexture>::Create(m_depthPyramidTexture),
                                                   nullptr,
                                                   ResourceState::UnorderedAccess,
                                                   ResourceState::ComputeShaderResource,
                                                   0,
                                                   0,
                                                   mip,
                                                   1 });
    }

    mutableCommandBuffer->Barrier(BarrierDesc{
        depthTexture, nullptr, ResourceState::ComputeShaderResource, ResourceState::DepthWrite, 0, 0, 0, 1 });

    m_depthPyramidValid = true;
}

} // namespace Yogi
