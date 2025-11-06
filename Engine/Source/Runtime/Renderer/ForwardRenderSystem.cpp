#include "Renderer/ForwardRenderSystem.h"
#include "Resources/AssetManager/AssetManager.h"
#include "Resources/ResourceManager/ResourceManager.h"
#include "Core/Application.h"
#include "Scene/World.h"
#include "Math/Vector4.h"

namespace Yogi
{

ForwardRenderSystem::ForwardRenderSystem()
{
    m_vertexBuffer = ResourceManager::GetResource<IBuffer>(
        BufferDesc{ MAX_VERTICES_SIZE, BufferUsage::Vertex, BufferAccess::Dynamic });
    m_indexBuffer = ResourceManager::GetResource<IBuffer>(
        BufferDesc{ MAX_INDICES * sizeof(uint32_t), BufferUsage::Index, BufferAccess::Dynamic });
    m_uniformBuffer = ResourceManager::GetResource<IBuffer>(
        BufferDesc{ sizeof(SceneData), BufferUsage::Uniform, BufferAccess::Dynamic });

    auto& swapChain = Application::GetInstance().GetSwapChain();

    m_renderPasses.push_back(AssetManager::GetAsset<IRenderPass>("EngineAssets/RenderPasses/Default.rp"));
    // m_renderPasses.push_back(ResourceManager::GetResource<IRenderPass>(RenderPassDesc{
    //     { AttachmentDesc{ swapChain->GetColorFormat(), AttachmentUsage::Present } },
    //     AttachmentDesc{ swapChain->GetDepthFormat(), AttachmentUsage::ShaderRead, LoadOp::Clear, StoreOp::DontCare },
    //     swapChain->GetNumSamples() }));

    m_shaderResourceBinding = ResourceManager::GetResource<IShaderResourceBinding>(std::vector<ShaderResourceAttribute>{
        ShaderResourceAttribute{ 0, 1, ShaderResourceType::Buffer, ShaderStage::Vertex } });
    m_shaderResourceBinding->BindBuffer(m_uniformBuffer, 0);
}

ForwardRenderSystem::~ForwardRenderSystem()
{
    m_vertexBuffer  = nullptr;
    m_indexBuffer   = nullptr;
    m_uniformBuffer = nullptr;
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
    m_uniformBuffer->UpdateData(&m_sceneData, sizeof(SceneData));

    // begin command buffer
    commandBuffer->Wait();
    BeginRender(commandBuffer, frameBuffer);
    uint32_t vertexOffset = 0;
    uint32_t indexOffset  = 0;

    // fill vertices and indices
    for (auto& renderPass : m_renderPasses)
    {
        world.ViewComponents<TransformComponent, MeshRendererComponent>(
            [&](Entity entity, TransformComponent& meshTransform, MeshRendererComponent& meshRenderer) {
                auto& mesh     = meshRenderer.Mesh;
                auto& material = meshRenderer.Material;
                if (!mesh || !material)
                    return;
                for (auto& materialPass : material->GetPasses())
                {
                    auto& pipeline   = materialPass.Pipeline;
                    auto& renderPass = pipeline->GetDesc().RenderPass;
                    if (renderPass != renderPass)
                        return;
                    std::vector<uint8_t>&  vertices = m_vertices[pipeline];
                    std::vector<uint32_t>& indices  = m_indices[pipeline];

                    std::vector<VertexAttribute> vertexLayout = pipeline->GetDesc().VertexLayout;
                    uint32_t                     vertexStride = vertexLayout.back().Offset + vertexLayout.back().Size;
                    if (vertices.size() + vertexStride * mesh->GetVertices().size() > MAX_VERTICES_SIZE ||
                        indices.size() + mesh->GetIndices().size() > MAX_INDICES)
                    {
                        commandBuffer->Wait();
                        Draw(pipeline, frameBuffer, vertexOffset, indexOffset);
                        EndRender(commandBuffer);
                        BeginRender(commandBuffer, frameBuffer);
                    }

                    size_t   oldSize        = vertices.size();
                    uint32_t baseVertex     = oldSize / vertexStride;
                    int      positionOffset = materialPass.PositionOffset;
                    int      normalOffset   = materialPass.NormalOffset;
                    int      texcoordOffset = materialPass.TexCoordOffset;
                    int      entityOffset   = materialPass.EntityOffset;
                    vertices.resize(oldSize + vertexStride * mesh->GetVertices().size());
                    uint8_t* verticesCur = vertices.data() + oldSize;

                    for (auto& vertex : mesh->GetVertices())
                    {
                        memcpy(verticesCur, materialPass.Data.data(), vertexStride);
                        if (positionOffset >= 0)
                        {
                            Vector4 position{ vertex.Position.x, vertex.Position.y, vertex.Position.z, 1.0f };
                            position = (Matrix4)(meshTransform.Transform) * position;
                            memcpy(verticesCur + positionOffset, &position, sizeof(float) * 3);
                        }
                        if (normalOffset >= 0)
                        {
                            Vector4 normal{ vertex.Normal.x, vertex.Normal.y, vertex.Normal.z, 0.0f };
                            normal = (Matrix4)(meshTransform.Transform) * normal;
                            memcpy(verticesCur + normalOffset, &normal, sizeof(float) * 3);
                        }
                        if (texcoordOffset >= 0)
                        {
                            memcpy(verticesCur + texcoordOffset, &vertex.Texcoord, sizeof(float) * 2);
                        }
                        if (entityOffset >= 0)
                        {
                            memcpy(verticesCur + entityOffset, &entity, sizeof(int));
                        }
                        verticesCur += vertexStride;
                    }

                    for (auto& index : mesh->GetIndices())
                    {
                        indices.push_back(baseVertex + index);
                    }
                }
            });
        for (auto& [pipeline, vertices] : m_vertices)
        {
            if (vertexOffset + vertices.size() > MAX_VERTICES_SIZE ||
                indexOffset + m_indices[pipeline].size() > MAX_INDICES)
            {
                EndRender(commandBuffer);
                commandBuffer->Wait();
                BeginRender(commandBuffer, frameBuffer);
                vertexOffset = 0;
                indexOffset  = 0;
            }
            Draw(pipeline, frameBuffer, vertexOffset, indexOffset);
        }
    }

    EndRender(commandBuffer);
}

void ForwardRenderSystem::Draw(const Ref<IPipeline>&    pipeline,
                               const Ref<IFrameBuffer>& frameBuffer,
                               uint32_t                 vertexOffset,
                               uint32_t                 indexOffset)
{
    YG_PROFILE_FUNCTION();

    std::vector<uint8_t>&  vertices = m_vertices[pipeline];
    std::vector<uint32_t>& indices  = m_indices[pipeline];
    m_vertexBuffer->UpdateData(vertices.data(), vertices.size() * sizeof(uint8_t), vertexOffset);
    m_indexBuffer->UpdateData(indices.data(), indices.size() * sizeof(uint32_t), indexOffset);
    vertices.clear();
    indices.clear();

    auto& swapChain = Application::GetInstance().GetSwapChain();

    Ref<ICommandBuffer> commandBuffer = swapChain->GetCurrentCommandBuffer();
    commandBuffer->SetPipeline(pipeline);
    commandBuffer->SetViewport({ 0, 0, (float)frameBuffer->GetWidth(), (float)frameBuffer->GetHeight() });
    commandBuffer->SetScissor({ 0, 0, frameBuffer->GetWidth(), frameBuffer->GetHeight() });
    commandBuffer->DrawIndexed(m_indexBuffer->GetSize() / sizeof(uint32_t), 1, 0, 0, 0);
}

void ForwardRenderSystem::BeginRender(Ref<ICommandBuffer>& commandBuffer, Ref<IFrameBuffer>& frameBuffer)
{
    commandBuffer->Begin();
    commandBuffer->BeginRenderPass(frameBuffer, { ClearValue{ 0.1f, 0.1f, 0.1f, 1.0f } }, ClearValue{ 1.0f, 0 });
    commandBuffer->SetVertexBuffer(m_vertexBuffer);
    commandBuffer->SetIndexBuffer(m_indexBuffer);
    commandBuffer->SetShaderResourceBinding(m_shaderResourceBinding);
}

void ForwardRenderSystem::EndRender(Ref<ICommandBuffer>& commandBuffer)
{
    commandBuffer->EndRenderPass();
    commandBuffer->End();
    commandBuffer->Submit();
}

} // namespace Yogi
