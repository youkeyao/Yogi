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
    void EndRender(Ref<ICommandBuffer>& commandBuffer);

private:
    static const uint32_t MAX_TRIANGLES      = 100000;
    static const uint32_t MAX_VERTICES       = MAX_TRIANGLES * 3;
    static const uint32_t MAX_VERTICES_SIZE  = MAX_VERTICES * sizeof(Vertex);
    static const uint32_t MAX_MESHLETS       = 10000;
    static const uint32_t MAX_MESHLET_SIZE   = MAX_MESHLETS * sizeof(Meshlet);

    SceneData m_sceneData;

    Ref<IBuffer> m_vertexStorageBuffer = nullptr;
    Ref<IBuffer> m_meshletBuffer       = nullptr;

    std::vector<Ref<IRenderPass>> m_renderPasses;
    Ref<IShaderResourceBinding>   m_shaderResourceBinding = nullptr;

    std::unordered_map<uint64_t, Ref<IFrameBuffer>> m_frameBuffers;
};

} // namespace Yogi