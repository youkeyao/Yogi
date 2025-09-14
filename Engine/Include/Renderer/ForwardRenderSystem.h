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

private:
    bool OnWindowResize(WindowResizeEvent& e, World& world);
    void RenderCamera(CameraComponent& camera, const TransformComponent& transform, World& world);
    void Flush(const Ref<IPipeline>& pipeline);

private:
    static const uint32_t MAX_TRIANGLES     = 100000;
    static const uint32_t MAX_VERTICES      = MAX_TRIANGLES * 3;
    static const uint32_t MAX_VERTICES_SIZE = MAX_VERTICES * 40;
    static const uint32_t MAX_INDICES       = MAX_TRIANGLES * 3;

    SceneData m_sceneData;

    Ref<IBuffer> m_vertexBuffer  = nullptr;
    Ref<IBuffer> m_indexBuffer   = nullptr;
    Ref<IBuffer> m_uniformBuffer = nullptr;

    Ref<IRenderPass>            m_renderPass            = nullptr;
    Ref<IShaderResourceBinding> m_shaderResourceBinding = nullptr;

    std::unordered_map<uint64_t, Ref<IFrameBuffer>>           m_frameBuffers;
    std::unordered_map<Ref<IPipeline>, std::vector<uint8_t>>  m_vertices;
    std::unordered_map<Ref<IPipeline>, std::vector<uint32_t>> m_indices;
};

} // namespace Yogi