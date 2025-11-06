#pragma once

#include "Scene/SystemBase.h"
#include "Events/ApplicationEvent.h"
#include "Renderer/RenderComponents.h"
#include "Renderer/RHI/ICommandBuffer.h"
#include "Renderer/RHI/IFrameBuffer.h"

namespace Yogi
{

class YG_API ForwardRenderSystem : public SystemBase
{
private:
    struct SceneData
    {
        Matrix4 ProjectionViewMatrix = Matrix4::Identity();
    };

public:
    ForwardRenderSystem();
    virtual ~ForwardRenderSystem();

    void OnUpdate(Timestep ts, World& world) override;
    void OnEvent(Event& e, World& world) override;

    void RenderCamera(const CameraComponent& camera, const TransformComponent& transform, World& world);

private:
    bool OnWindowResize(WindowResizeEvent& e, World& world);
    void BeginRender(Ref<ICommandBuffer>& commandBuffer, Ref<IFrameBuffer>& frameBuffer);
    void Draw(const Ref<IPipeline>&    pipeline,
              const Ref<IFrameBuffer>& frameBuffer,
              uint32_t                 vertexOffset,
              uint32_t                 indexOffset);
    void EndRender(Ref<ICommandBuffer>& commandBuffer);

private:
    static const uint32_t MAX_TRIANGLES     = 100000;
    static const uint32_t MAX_VERTICES      = MAX_TRIANGLES * 3;
    static const uint32_t MAX_VERTICES_SIZE = MAX_VERTICES * 40;
    static const uint32_t MAX_INDICES       = MAX_TRIANGLES * 3;

    SceneData m_sceneData;

    Ref<IBuffer> m_vertexBuffer  = nullptr;
    Ref<IBuffer> m_indexBuffer   = nullptr;
    Ref<IBuffer> m_uniformBuffer = nullptr;

    std::vector<Ref<IRenderPass>> m_renderPasses;
    Ref<IShaderResourceBinding>   m_shaderResourceBinding = nullptr;

    std::unordered_map<uint64_t, Ref<IFrameBuffer>>           m_frameBuffers;
    std::unordered_map<Ref<IPipeline>, std::vector<uint8_t>>  m_vertices;
    std::unordered_map<Ref<IPipeline>, std::vector<uint32_t>> m_indices;
};

} // namespace Yogi