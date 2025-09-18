#include "Renderer/ForwardRenderSystem.h"
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
    m_indexBuffer =
        ResourceManager::GetResource<IBuffer>(BufferDesc{ MAX_INDICES, BufferUsage::Index, BufferAccess::Dynamic });
    m_uniformBuffer = ResourceManager::GetResource<IBuffer>(
        BufferDesc{ sizeof(SceneData), BufferUsage::Uniform, BufferAccess::Dynamic });

    auto& swapChain = Application::GetInstance().GetSwapChain();

    m_renderPass = ResourceManager::GetResource<IRenderPass>(RenderPassDesc{
        { AttachmentDesc{ swapChain->GetColorFormat(), AttachmentUsage::Present } },
        AttachmentDesc{ swapChain->GetDepthFormat(), AttachmentUsage::DepthStencil, LoadOp::Clear, StoreOp::DontCare },
        swapChain->GetNumSamples() });

    m_shaderResourceBinding = ResourceManager::GetResource<IShaderResourceBinding>(std::vector<ShaderResourceAttribute>{
        ShaderResourceAttribute{ 0, 1, ShaderResourceType::Buffer, ShaderStage::Vertex } });
    m_shaderResourceBinding->BindBuffer(m_uniformBuffer, 0);

    m_commandBuffer = ResourceManager::GetResource<ICommandBuffer>(
        CommandBufferDesc{ CommandBufferUsage::Persistent, SubmitQueue::Graphics });
}

ForwardRenderSystem::~ForwardRenderSystem()
{
    m_commandBuffer->Wait();
    m_commandBuffer = nullptr;
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
    m_commandBuffer->Wait();
    m_frameBuffers.clear();
    return false;
}

void ForwardRenderSystem::RenderCamera(CameraComponent& camera, const TransformComponent& transform, World& world)
{
    YG_PROFILE_FUNCTION();

    auto& swapChain = Application::GetInstance().GetSwapChain();

    Matrix4 viewMatrix = MathUtils::Inverse(transform.Transform);
    Matrix4 projectionMatrix;
    camera.AspectRatio = (float)swapChain->GetWidth() / (float)swapChain->GetHeight();
    if (camera.IsOrtho)
    {
        projectionMatrix = MathUtils::Orthographic(-camera.AspectRatio * camera.ZoomLevel,
                                                   camera.AspectRatio * camera.ZoomLevel,
                                                   -camera.ZoomLevel,
                                                   camera.ZoomLevel,
                                                   -1.0f,
                                                   1.0f);
    }
    else
    {
        projectionMatrix = MathUtils::Perspective(camera.Fov, camera.AspectRatio, 0.1f, 100.0f);
    }

    m_sceneData.ProjectionViewMatrix = projectionMatrix * viewMatrix;
    m_uniformBuffer->UpdateData(&m_sceneData, sizeof(SceneData));

    // fill vertices and indices
    world.ViewComponents<TransformComponent, MeshRendererComponent>(
        [&](Entity entity, TransformComponent& transform, MeshRendererComponent& meshRenderer) {
            auto& mesh     = meshRenderer.Mesh;
            auto& material = meshRenderer.Material;
            auto  pipeline = material->GetPipeline();

            std::vector<uint8_t>&  vertices = m_vertices[pipeline];
            std::vector<uint32_t>& indices  = m_indices[pipeline];

            std::vector<VertexAttribute> vertexLayout = pipeline->GetVertexLayout();
            uint32_t                     vertexStride = vertexLayout.back().Offset + vertexLayout.back().Size;
            if (vertices.size() + vertexStride * mesh->GetVertices().size() > MAX_VERTICES_SIZE)
            {
                Flush(pipeline);
            }

            size_t   oldSize        = vertices.size();
            uint32_t baseVertex     = oldSize / vertexStride;
            int      positionOffset = material->GetPositionOffset();
            int      normalOffset   = material->GetNormalOffset();
            int      texcoordOffset = material->GetTexCoordOffset();
            int      entityOffset   = material->GetEntityOffset();
            vertices.resize(oldSize + vertexStride * mesh->GetVertices().size());
            uint8_t* verticesCur = vertices.data() + oldSize;

            for (auto& vertex : mesh->GetVertices())
            {
                if (positionOffset >= 0)
                {
                    Vector4 position{ vertex.Position.x, vertex.Position.y, vertex.Position.z, 1.0f };
                    position = (Matrix4)(transform.Transform) * position;
                    memcpy(verticesCur + positionOffset, &position, sizeof(float) * 3);
                }
                if (normalOffset >= 0)
                {
                    Vector4 normal{ vertex.Normal.x, vertex.Normal.y, vertex.Normal.z, 0.0f };
                    normal = (Matrix4)(transform.Transform) * normal;
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
        });

    for (auto& [pipeline, vertices] : m_vertices)
    {
        Flush(pipeline);
    }
}

void ForwardRenderSystem::Flush(const Ref<IPipeline>& pipeline)
{
    YG_PROFILE_FUNCTION();

    std::vector<uint8_t>&  vertices = m_vertices[pipeline];
    std::vector<uint32_t>& indices  = m_indices[pipeline];
    m_vertexBuffer->UpdateData(vertices.data(), vertices.size() * sizeof(uint8_t));
    m_indexBuffer->UpdateData(indices.data(), indices.size() * sizeof(uint32_t));
    vertices.clear();
    indices.clear();

    auto& swapChain = Application::GetInstance().GetSwapChain();

    auto            currentTarget = swapChain->GetCurrentTarget();
    auto            currentDepth  = swapChain->GetCurrentDepth();
    FrameBufferDesc desc{
        swapChain->GetWidth(), swapChain->GetHeight(), m_renderPass, { currentTarget }, currentDepth,
    };
    uint64_t key = HashArgs(desc);
    auto     it  = m_frameBuffers.find(key);
    if (it == m_frameBuffers.end())
    {
        it = m_frameBuffers.insert({ key, ResourceManager::GetResource<IFrameBuffer>(desc) }).first;
    }
    auto& frameBuffer = it->second;

    m_commandBuffer->Wait();
    m_commandBuffer->Begin();
    m_commandBuffer->BeginRenderPass(frameBuffer, { ClearValue{ 0.1f, 0.1f, 0.1f, 1.0f } }, ClearValue{ 1.0f, 0 });
    m_commandBuffer->SetVertexBuffer(m_vertexBuffer);
    m_commandBuffer->SetIndexBuffer(m_indexBuffer);
    m_commandBuffer->SetPipeline(pipeline);
    m_commandBuffer->SetShaderResourceBinding(m_shaderResourceBinding);
    m_commandBuffer->SetViewport({ 0, 0, (float)swapChain->GetWidth(), (float)swapChain->GetHeight() });
    m_commandBuffer->SetScissor({ 0, 0, swapChain->GetWidth(), swapChain->GetHeight() });
    m_commandBuffer->DrawIndexed(m_indexBuffer->GetSize() / sizeof(uint32_t), 1, 0, 0, 0);
    m_commandBuffer->EndRenderPass();
    m_commandBuffer->End();
    m_commandBuffer->Submit();
}

} // namespace Yogi
