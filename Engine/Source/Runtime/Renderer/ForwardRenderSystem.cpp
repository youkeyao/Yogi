#include "Renderer/ForwardRenderSystem.h"
#include "Core/Application.h"
#include "Scene/World.h"
#include "Math/Vector4.h"

namespace Yogi
{

ForwardRenderSystem::ForwardRenderSystem()
{
    m_vertexBuffer  = IBuffer::Create(BufferDesc{ MAX_VERTICES_SIZE, BufferUsage::Vertex, BufferAccess::Dynamic });
    m_indexBuffer   = IBuffer::Create(BufferDesc{ MAX_INDICES, BufferUsage::Index, BufferAccess::Dynamic });
    m_uniformBuffer = IBuffer::Create(BufferDesc{ sizeof(SceneData), BufferUsage::Uniform, BufferAccess::Dynamic });

    auto& swapChain = Application::GetInstance().GetSwapChain();

    m_renderPass = Yogi::IRenderPass::Create(
        Yogi::RenderPassDesc{ { Yogi::AttachmentDesc{ swapChain->GetColorFormat(), Yogi::AttachmentUsage::Present } },
                              Yogi::AttachmentDesc{ swapChain->GetDepthFormat(),
                                                    Yogi::AttachmentUsage::DepthStencil,
                                                    Yogi::LoadOp::Clear,
                                                    Yogi::StoreOp::DontCare },
                              swapChain->GetNumSamples() });
    m_shaderResourceBinding = Yogi::IShaderResourceBinding::Create(
        { Yogi::ShaderResourceAttribute{ 0, 1, Yogi::ShaderResourceType::Buffer, Yogi::ShaderStage::Vertex } });
    m_shaderResourceBinding->BindBuffer(Ref<IBuffer>::Create(m_uniformBuffer), 0);
}

ForwardRenderSystem::~ForwardRenderSystem()
{
    m_vertexBuffer  = nullptr;
    m_indexBuffer   = nullptr;
    m_uniformBuffer = nullptr;
    m_frameBuffer   = nullptr;
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
    dispatcher.dispatch<WindowResizeEvent>(YG_BIND_FN(ForwardRenderSystem::OnWindowResize, std::placeholders::_2),
                                           world);
}

bool ForwardRenderSystem::OnWindowResize(WindowResizeEvent& e, World& world)
{
    auto& swapChain     = Application::GetInstance().GetSwapChain();
    auto  currentTarget = swapChain->GetCurrentTarget();
    auto  currentDepth  = swapChain->GetCurrentDepth();

    m_frameBuffer = IFrameBuffer::Create(FrameBufferDesc{
        swapChain->GetWidth(),
        swapChain->GetHeight(),
        Ref<IRenderPass>::Create(m_renderPass),
        { currentTarget },
        currentDepth,
    });
    return false;
}

void ForwardRenderSystem::RenderCamera(CameraComponent& camera, const TransformComponent& transform, World& world)
{
    auto& swapChain     = Application::GetInstance().GetSwapChain();
    
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
    
    auto  currentTarget = swapChain->GetCurrentTarget();
    auto  currentDepth  = swapChain->GetCurrentDepth();
    m_frameBuffer       = IFrameBuffer::Create(FrameBufferDesc{
        swapChain->GetWidth(),
        swapChain->GetHeight(),
        Ref<IRenderPass>::Create(m_renderPass),
              { currentTarget },
        currentDepth,
    });

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
    std::vector<uint8_t>&  vertices = m_vertices[pipeline];
    std::vector<uint32_t>& indices  = m_indices[pipeline];
    m_vertexBuffer->UpdateData(vertices.data(), vertices.size() * sizeof(uint8_t));
    m_indexBuffer->UpdateData(indices.data(), indices.size() * sizeof(uint32_t));
    vertices.clear();
    indices.clear();

    auto&                  swapChain = Application::GetInstance().GetSwapChain();
    Handle<ICommandBuffer> commandBuffer =
        ICommandBuffer::Create(CommandBufferDesc{ CommandBufferUsage::OneTimeSubmit, SubmitQueue::Graphics });

    commandBuffer->Begin();
    commandBuffer->BeginRenderPass(Ref<IFrameBuffer>::Create(m_frameBuffer),
                                   { Yogi::ClearValue{ 0.1f, 0.1f, 0.1f, 1.0f } },
                                   Yogi::ClearValue{ 1.0f, 0 });
    commandBuffer->SetVertexBuffer(Ref<IBuffer>::Create(m_vertexBuffer));
    commandBuffer->SetIndexBuffer(Ref<IBuffer>::Create(m_indexBuffer));
    commandBuffer->SetPipeline(pipeline);
    commandBuffer->SetShaderResourceBinding(Ref<IShaderResourceBinding>::Create(m_shaderResourceBinding));
    commandBuffer->SetViewport({ 0, 0, (float)swapChain->GetWidth(), (float)swapChain->GetHeight() });
    commandBuffer->SetScissor({ 0, 0, swapChain->GetWidth(), swapChain->GetHeight() });
    commandBuffer->DrawIndexed(m_indexBuffer->GetSize() / sizeof(uint32_t), 1, 0, 0, 0);
    commandBuffer->EndRenderPass();
    commandBuffer->End();
    commandBuffer->Submit();
}

} // namespace Yogi
