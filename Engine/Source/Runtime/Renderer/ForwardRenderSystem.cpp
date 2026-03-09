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

    auto& swapChain = Application::GetInstance().GetSwapChain();

    m_renderPasses.push_back(AssetManager::GetAsset<IRenderPass>("EngineAssets/RenderPasses/Default.rp"));

    m_shaderResourceBinding = ResourceManager::GetResource<IShaderResourceBinding>(std::vector<ShaderResourceAttribute>{
        ShaderResourceAttribute{ 0, 1, ShaderResourceType::StorageBuffer, ShaderStage::Mesh },
        ShaderResourceAttribute{ 1, 1, ShaderResourceType::StorageBuffer, ShaderStage::Mesh } });
    m_shaderResourceBinding->BindBuffer(m_vertexStorageBuffer, 0);
    m_shaderResourceBinding->BindBuffer(m_meshletBuffer, 1);
}

ForwardRenderSystem::~ForwardRenderSystem()
{
    m_vertexStorageBuffer = nullptr;
    m_meshletBuffer       = nullptr;
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

    constexpr uint32_t maxVertexCount = MAX_VERTICES_SIZE / sizeof(VertexData);

    std::vector<VertexData>                                      batchVertices;
    std::unordered_map<Ref<IPipeline>, std::vector<MeshletData>> pipelineMeshlets;

    auto flushBatch = [&]() {
        if (batchVertices.empty())
            return;
        m_vertexStorageBuffer->UpdateData(batchVertices.data(), batchVertices.size() * sizeof(VertexData), 0);
        for (auto& [pipeline, meshlets] : pipelineMeshlets)
        {
            m_meshletBuffer->UpdateData(meshlets.data(), meshlets.size() * sizeof(MeshletData), 0);
            commandBuffer->SetPipeline(pipeline);
            commandBuffer->SetViewport({ 0, 0, (float)frameBuffer->GetWidth(), (float)frameBuffer->GetHeight() });
            commandBuffer->SetScissor({ 0, 0, frameBuffer->GetWidth(), frameBuffer->GetHeight() });
            commandBuffer->DrawMeshTasks(static_cast<uint32_t>(meshlets.size()), 1, 1);
        }
        batchVertices.clear();
        pipelineMeshlets.clear();
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

            // Count total meshlets in current batch
            uint32_t totalMeshlets = 0;
            for (auto& [p, m] : pipelineMeshlets)
                totalMeshlets += static_cast<uint32_t>(m.size());

            // flush if adding this mesh would exceed buffer capacity
            if (batchVertices.size() + meshVertexCount > maxVertexCount ||
                totalMeshlets + meshMeshletCount > MAX_MESHLETS)
            {
                flushBatch();
                EndRender(commandBuffer);
                commandBuffer->Wait();
                BeginRender(commandBuffer, frameBuffer);
            }

            uint32_t vertexBase = static_cast<uint32_t>(batchVertices.size());

            // Append vertices
            batchVertices.insert(batchVertices.end(), mesh->GetVertices().begin(), mesh->GetVertices().end());
            // Append meshlets with offset vertex indices
            for (auto& materialPass : material->GetPasses())
            {
                auto& pipeline = materialPass.Pipeline;
                auto& meshlets = pipelineMeshlets[pipeline];
                for (const auto& srcMeshlet : mesh->GetMeshlets())
                {
                    MeshletData offsetMeshlet = srcMeshlet;
                    for (uint8_t i = 0; i < srcMeshlet.VertexCount; ++i)
                    {
                        offsetMeshlet.Vertices[i] += vertexBase;
                    }
                    meshlets.push_back(offsetMeshlet);
                }
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
