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
    void RenderCamera(const CameraComponent& camera, const TransformComponent& transform, World& world);
    void Flush(const View<IPipeline>& pipeline);

private:
    static const uint32_t MAX_TRIANGLES     = 100000;
    static const uint32_t MAX_VERTICES      = MAX_TRIANGLES * 3;
    static const uint32_t MAX_VERTICES_SIZE = MAX_VERTICES * 40;
    static const uint32_t MAX_INDICES       = MAX_TRIANGLES * 3;

    SceneData m_sceneData;

    Scope<IBuffer> m_vertexBuffer;
    Scope<IBuffer> m_indexBuffer;
    Scope<IBuffer> m_uniformBuffer;

    Scope<IRenderPass>            m_renderPass;
    Scope<IShaderResourceBinding> m_shaderResourceBinding;
    Scope<IFrameBuffer>           m_frameBuffer;

    std::unordered_map<View<IPipeline>, std::vector<uint8_t>>  m_vertices;
    std::unordered_map<View<IPipeline>, std::vector<uint32_t>> m_indices;
};

} // namespace Yogi