#include "Renderer/ForwardRenderSystem.h"
#include "Resources/AssetManager/AssetManager.h"
#include "Resources/ResourceManager/ResourceManager.h"
#include "Core/Application.h"
#include "Scene/World.h"
#include "Math/Vector.h"

#include <unordered_map>

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
    m_meshDrawBuffer = ResourceManager::GetResource<IBuffer>(
        BufferDesc{ MAX_MESH_DRAW_SIZE, BufferUsage::Storage, BufferAccess::Dynamic });
    m_meshTaskIndirectBuffer = ResourceManager::GetResource<IBuffer>(
        BufferDesc{ MAX_INDIRECT_DRAW_COMMAND_SIZE, BufferUsage::Indirect, BufferAccess::Dynamic });

    auto& swapChain = Application::GetInstance().GetSwapChain();

    m_renderPasses.push_back(AssetManager::GetAsset<IRenderPass>("EngineAssets/RenderPasses/Default.rp"));

    m_shaderResourceBinding = ResourceManager::GetResource<IShaderResourceBinding>(
        std::vector<ShaderResourceAttribute>{
            ShaderResourceAttribute{ 0, 1, ShaderResourceType::StorageBuffer, ShaderStage::Mesh },
            ShaderResourceAttribute{ 1, 1, ShaderResourceType::StorageBuffer, ShaderStage::Task | ShaderStage::Mesh },
            ShaderResourceAttribute{ 2, 1, ShaderResourceType::StorageBuffer, ShaderStage::Mesh },
            ShaderResourceAttribute{ 3, 1, ShaderResourceType::StorageBuffer, ShaderStage::Task | ShaderStage::Mesh } },
        std::vector<PushConstantRange>{
            PushConstantRange{ ShaderStage::Task | ShaderStage::Mesh, 0, static_cast<uint32_t>(sizeof(SceneData)) } });

    m_shaderResourceBinding->BindBuffer(m_vertexStorageBuffer, 0);
    m_shaderResourceBinding->BindBuffer(m_meshletBuffer, 1);
    m_shaderResourceBinding->BindBuffer(m_meshletDataBuffer, 2);
    m_shaderResourceBinding->BindBuffer(m_meshDrawBuffer, 3);
}

ForwardRenderSystem::~ForwardRenderSystem()
{
    m_vertexStorageBuffer    = nullptr;
    m_meshletBuffer          = nullptr;
    m_meshletDataBuffer      = nullptr;
    m_meshDrawBuffer         = nullptr;
    m_meshTaskIndirectBuffer = nullptr;
    m_shaderResourceBinding  = nullptr;
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

    constexpr uint32_t maxVertexCount = MAX_VERTICES_SIZE / sizeof(VertexData);

    struct RenderBatch
    {
        Ref<IPipeline>           Pipeline;
        std::vector<MeshletData> Meshlets;
        std::vector<uint32_t>    MeshletData;
        std::vector<MeshDraw>    MeshDraws;
        std::vector<uint32_t>    IndirectCommands;
    };

    std::vector<VertexData>           batchVertices;
    std::vector<RenderBatch>          renderBatches;
    std::unordered_map<uint64_t, int> renderBatchLookup;

    auto flushBatch = [&]() {
        if (batchVertices.empty())
            return;

        m_vertexStorageBuffer->UpdateData(batchVertices.data(), batchVertices.size() * sizeof(VertexData), 0);
        for (auto& batch : renderBatches)
        {
            if (!batch.Pipeline)
                continue;
            if (batch.MeshDraws.empty())
                continue;

            m_meshletBuffer->UpdateData(batch.Meshlets.data(), batch.Meshlets.size() * sizeof(MeshletData), 0);
            m_meshletDataBuffer->UpdateData(batch.MeshletData.data(), batch.MeshletData.size() * sizeof(uint32_t), 0);
            m_meshDrawBuffer->UpdateData(batch.MeshDraws.data(), batch.MeshDraws.size() * sizeof(MeshDraw), 0);
            m_meshTaskIndirectBuffer->UpdateData(
                batch.IndirectCommands.data(), batch.IndirectCommands.size() * sizeof(uint32_t), 0);

            commandBuffer->SetPipeline(batch.Pipeline);

            commandBuffer->SetViewport({ 0, 0, (float)frameBuffer->GetWidth(), (float)frameBuffer->GetHeight() });
            commandBuffer->SetScissor({ 0, 0, frameBuffer->GetWidth(), frameBuffer->GetHeight() });
            uint32_t           drawCount     = static_cast<uint32_t>(batch.MeshDraws.size());
            constexpr uint32_t commandStride = sizeof(uint32_t) * 3;
            commandBuffer->SetPushConstants(
                m_shaderResourceBinding, ShaderStage::Task | ShaderStage::Mesh, 0, sizeof(SceneData), &m_sceneData);
            commandBuffer->DrawMeshTasksIndirect(m_meshTaskIndirectBuffer, 0, drawCount, commandStride);
        }
        batchVertices.clear();
        renderBatches.clear();
        renderBatchLookup.clear();
    };

    BeginRender(commandBuffer, frameBuffer);

    // collect and batch meshes into meshlets
    world.ViewComponents<TransformComponent, MeshRendererComponent>(
        [&](Entity entity, TransformComponent& meshTransform, MeshRendererComponent& meshRenderer) {
            auto& mesh     = meshRenderer.Mesh;
            auto& material = meshRenderer.Material;
            if (!mesh || !material)
                return;

            uint32_t meshVertexCount  = static_cast<uint32_t>(mesh->GetVertices().size());
            uint32_t meshMeshletCount = static_cast<uint32_t>(mesh->GetMeshlets().size());
            uint32_t passCount        = static_cast<uint32_t>(material->GetPasses().size());

            // Count total meshlets in current batch
            uint32_t totalMeshlets  = 0;
            uint32_t totalMeshDraws = 0;
            for (const auto& batch : renderBatches)
            {
                totalMeshlets += static_cast<uint32_t>(batch.Meshlets.size());
                totalMeshDraws += static_cast<uint32_t>(batch.MeshDraws.size());
            }

            // flush if adding this mesh would exceed buffer capacity
            if (batchVertices.size() + meshVertexCount > maxVertexCount ||
                totalMeshlets + meshMeshletCount * passCount > MAX_MESHLETS ||
                totalMeshDraws + passCount > MAX_MESH_DRAWS)
            {
                flushBatch();
                EndRender(commandBuffer);
                commandBuffer->Wait();
                BeginRender(commandBuffer, frameBuffer);
            }

            uint32_t vertexBase = static_cast<uint32_t>(batchVertices.size());
            // Append vertices
            batchVertices.insert(batchVertices.end(), mesh->GetVertices().begin(), mesh->GetVertices().end());
            // Append meshlets grouped by material pass.
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
                    renderBatches.push_back(RenderBatch{ pipeline, {}, {} });
                    lookupIt = renderBatchLookup.find(passKey);
                }

                auto& batch = renderBatches[lookupIt->second];

                MeshDraw draw{};
                draw.Position        = meshTransform.Transform.Position;
                draw.Scale           = meshTransform.Transform.Scale.x;
                draw.Orientation     = Vector4(meshTransform.Transform.Rotation.x,
                                           meshTransform.Transform.Rotation.y,
                                           meshTransform.Transform.Rotation.z,
                                           meshTransform.Transform.Rotation.w);
                draw.MeshletDataBase = static_cast<uint32_t>(batch.MeshletData.size());
                draw.MeshletOffset   = static_cast<uint32_t>(batch.Meshlets.size());
                draw.MeshletCount    = meshMeshletCount;
                draw.VertexOffset    = vertexBase;
                batch.MeshDraws.push_back(draw);

                uint32_t taskWorkGroupCount = (meshMeshletCount + TASK_WGSIZE - 1) / TASK_WGSIZE;
                batch.IndirectCommands.push_back(taskWorkGroupCount);
                batch.IndirectCommands.push_back(1);
                batch.IndirectCommands.push_back(1);

                batch.Meshlets.insert(batch.Meshlets.end(), mesh->GetMeshlets().begin(), mesh->GetMeshlets().end());
                batch.MeshletData.insert(
                    batch.MeshletData.end(), mesh->GetMeshletData().begin(), mesh->GetMeshletData().end());
            }
        });

    flushBatch();
    EndRender(commandBuffer);
}

void ForwardRenderSystem::BeginRender(Ref<ICommandBuffer>& commandBuffer, Ref<IFrameBuffer>& frameBuffer)
{
    commandBuffer->Begin();
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
