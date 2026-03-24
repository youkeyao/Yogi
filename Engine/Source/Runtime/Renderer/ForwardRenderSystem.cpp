#include "Renderer/ForwardRenderSystem.h"
#include "Resources/AssetManager/AssetManager.h"
#include "Resources/ResourceManager/ResourceManager.h"
#include "Core/Application.h"
#include "Scene/World.h"
#include "Math/Vector.h"

namespace Yogi
{

ForwardRenderSystem::ForwardRenderSystem()
{
    m_vertexStorageBuffer = ResourceManager::GetResource<IBuffer>(
        BufferDesc{ MAX_VERTICES_SIZE, BufferUsage::Storage, BufferAccess::Dynamic });
    m_meshletBuffer = ResourceManager::GetResource<IBuffer>(
        BufferDesc{ MAX_MESHLET_SIZE, BufferUsage::Storage, BufferAccess::Dynamic });
    m_meshletDataBuffer = ResourceManager::GetResource<IBuffer>(
        BufferDesc{ MAX_MESHLET_DATA_SIZE, BufferUsage::Storage, BufferAccess::Dynamic });
    m_meshBuffer =
        ResourceManager::GetResource<IBuffer>(BufferDesc{ MAX_MESH_SIZE, BufferUsage::Storage, BufferAccess::Dynamic });
    m_meshDrawBuffer = ResourceManager::GetResource<IBuffer>(
        BufferDesc{ MAX_MESH_DRAW_SIZE, BufferUsage::Storage, BufferAccess::Dynamic });
    m_meshTaskIndirectBuffer = ResourceManager::GetResource<IBuffer>(BufferDesc{
        MAX_INDIRECT_DRAW_COMMAND_SIZE, BufferUsage::Storage | BufferUsage::Indirect, BufferAccess::Dynamic });
    m_visibleDrawIndexBuffer = ResourceManager::GetResource<IBuffer>(
        BufferDesc{ MAX_VISIBLE_DRAW_INDEX_SIZE, BufferUsage::Storage, BufferAccess::Dynamic });
    m_meshTaskIndirectCountBuffer = ResourceManager::GetResource<IBuffer>(BufferDesc{
        MAX_INDIRECT_DRAW_COUNT_SIZE, BufferUsage::Storage | BufferUsage::Indirect, BufferAccess::Dynamic });

    m_renderPasses.push_back(AssetManager::GetAsset<IRenderPass>("EngineAssets/RenderPasses/Default.rp"));

    m_shaderResourceBinding = ResourceManager::GetResource<IShaderResourceBinding>(
        std::vector<ShaderResourceAttribute>{
            ShaderResourceAttribute{ 0, 1, ShaderResourceType::StorageBuffer, ShaderStage::Mesh },
            ShaderResourceAttribute{ 1, 1, ShaderResourceType::StorageBuffer, ShaderStage::Task | ShaderStage::Mesh },
            ShaderResourceAttribute{ 2, 1, ShaderResourceType::StorageBuffer, ShaderStage::Mesh },
            ShaderResourceAttribute{ 3, 1, ShaderResourceType::StorageBuffer, ShaderStage::Task | ShaderStage::Mesh },
            ShaderResourceAttribute{ 4, 1, ShaderResourceType::StorageBuffer, ShaderStage::Task | ShaderStage::Mesh },
            ShaderResourceAttribute{ 5, 1, ShaderResourceType::StorageBuffer, ShaderStage::Task } },
        std::vector<PushConstantRange>{
            PushConstantRange{ ShaderStage::Task | ShaderStage::Mesh, 0, static_cast<uint32_t>(sizeof(SceneData)) } });

    m_shaderResourceBinding->BindBuffer(m_vertexStorageBuffer, 0);
    m_shaderResourceBinding->BindBuffer(m_meshletBuffer, 1);
    m_shaderResourceBinding->BindBuffer(m_meshletDataBuffer, 2);
    m_shaderResourceBinding->BindBuffer(m_meshBuffer, 3);
    m_shaderResourceBinding->BindBuffer(m_meshDrawBuffer, 4);
    m_shaderResourceBinding->BindBuffer(m_visibleDrawIndexBuffer, 5);

    m_cullShaderResourceBinding = ResourceManager::GetResource<IShaderResourceBinding>(
        std::vector<ShaderResourceAttribute>{
            ShaderResourceAttribute{ 0, 1, ShaderResourceType::StorageBuffer, ShaderStage::Compute },
            ShaderResourceAttribute{ 1, 1, ShaderResourceType::StorageBuffer, ShaderStage::Compute },
            ShaderResourceAttribute{ 2, 1, ShaderResourceType::StorageBuffer, ShaderStage::Compute },
            ShaderResourceAttribute{ 3, 1, ShaderResourceType::StorageBuffer, ShaderStage::Compute },
            ShaderResourceAttribute{ 4, 1, ShaderResourceType::StorageBuffer, ShaderStage::Compute } },
        std::vector<PushConstantRange>{
            PushConstantRange{ ShaderStage::Compute, 0, static_cast<uint32_t>(sizeof(CullData)) } });
    m_cullShaderResourceBinding->BindBuffer(m_meshBuffer, 0);
    m_cullShaderResourceBinding->BindBuffer(m_meshDrawBuffer, 1);
    m_cullShaderResourceBinding->BindBuffer(m_meshTaskIndirectBuffer, 2);
    m_cullShaderResourceBinding->BindBuffer(m_visibleDrawIndexBuffer, 3);
    m_cullShaderResourceBinding->BindBuffer(m_meshTaskIndirectCountBuffer, 4);

    PipelineDesc cullPipelineDesc{};
    cullPipelineDesc.Type    = PipelineType::Compute;
    cullPipelineDesc.Shaders = { AssetManager::GetAsset<ShaderDesc>("EngineAssets/Shaders/ObjectCull.comp") };
    cullPipelineDesc.ShaderResourceBinding = m_cullShaderResourceBinding;
    m_cullPipeline                         = ResourceManager::GetResource<IPipeline>(cullPipelineDesc);
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

    auto&               swapChain     = Application::GetInstance().GetSwapChain();
    Ref<ICommandBuffer> commandBuffer = swapChain->GetCurrentCommandBuffer();
    auto                currentTarget = camera.Target ? camera.Target : swapChain->GetCurrentTarget();
    auto                currentDepth  = swapChain->GetCurrentDepth();
    FrameBufferDesc     desc{
        currentTarget->GetWidth(), currentTarget->GetHeight(), m_renderPasses[0], { currentTarget }, currentDepth,
    };
    uint64_t key = HashArgs(desc);
    auto     it  = m_frameBuffers.find(key);
    if (it == m_frameBuffers.end())
    {
        it = m_frameBuffers.insert({ key, ResourceManager::GetResource<IFrameBuffer>(desc) }).first;
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
        Ref<IPipeline>        Pipeline;
        std::vector<MeshDraw> MeshDraws;
        uint32_t              DrawBase            = 0;
        uint32_t              DrawCount           = 0;
        uint32_t              IndirectOffsetBytes = 0;
    };

    std::vector<RenderBatch>          renderBatches;
    std::unordered_map<uint64_t, int> renderBatchLookup;

    auto flushBatch = [&]() {
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

            commandBuffer->SetPipeline(m_cullPipeline);
            commandBuffer->SetShaderResourceBinding(m_cullShaderResourceBinding);

            constexpr uint32_t CULL_WORKGROUP_SIZE = 64;
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
                    m_cullShaderResourceBinding, ShaderStage::Compute, 0, sizeof(CullData), &cullData);

                uint32_t dispatchX = (batch.DrawCount + CULL_WORKGROUP_SIZE - 1) / CULL_WORKGROUP_SIZE;
                commandBuffer->Dispatch(dispatchX, 1, 1);
            }
            if (totalDrawCount > 0)
                commandBuffer->Barrier(PipelineStage::ComputeShader,
                                       PipelineStage::DrawIndirect | PipelineStage::TaskShader |
                                           PipelineStage::MeshShader,
                                       BarrierAccess::ShaderWrite,
                                       BarrierAccess::IndirectCommandRead | BarrierAccess::ShaderRead);
        }

        BeginRender(commandBuffer, frameBuffer);
        commandBuffer->SetViewport({ 0, 0, (float)frameBuffer->GetWidth(), (float)frameBuffer->GetHeight() });
        commandBuffer->SetScissor({ 0, 0, frameBuffer->GetWidth(), frameBuffer->GetHeight() });

        for (size_t batchIndex = 0; batchIndex < renderBatches.size(); ++batchIndex)
        {
            auto& batch = renderBatches[batchIndex];
            if (!batch.Pipeline || batch.DrawCount == 0)
                continue;

            commandBuffer->SetPipeline(batch.Pipeline);
            commandBuffer->SetShaderResourceBinding(m_shaderResourceBinding);

            SceneData drawSceneData = m_sceneData;
            drawSceneData.DrawBase  = batch.DrawBase;
            commandBuffer->SetPushConstants(
                m_shaderResourceBinding, ShaderStage::Task | ShaderStage::Mesh, 0, sizeof(SceneData), &drawSceneData);
            commandBuffer->DrawMeshTasksIndirectCount(m_meshTaskIndirectBuffer,
                                                      batch.IndirectOffsetBytes,
                                                      m_meshTaskIndirectCountBuffer,
                                                      static_cast<uint32_t>(batchIndex * sizeof(uint32_t)),
                                                      batch.DrawCount,
                                                      sizeof(uint32_t) * 3);
        }

        EndRender(commandBuffer);

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
            commandBuffer->Wait();
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
                commandBuffer->Wait();
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
            Ref<IPipeline> pipeline = materialPass.Pipeline;
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

void ForwardRenderSystem::BeginRender(Ref<ICommandBuffer>& commandBuffer, Ref<IFrameBuffer>& frameBuffer)
{
    commandBuffer->BeginRenderPass(frameBuffer, { ClearValue{ 0.1f, 0.1f, 0.1f, 1.0f } }, ClearValue{ 1.0f, 0 });
    commandBuffer->SetShaderResourceBinding(m_shaderResourceBinding);
}

void ForwardRenderSystem::EndRender(Ref<ICommandBuffer>& commandBuffer)
{
    commandBuffer->EndRenderPass();
    commandBuffer->End();
    commandBuffer->Submit();
}

} // namespace Yogi
