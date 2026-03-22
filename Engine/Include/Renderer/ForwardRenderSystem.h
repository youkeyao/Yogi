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
    static const uint32_t MAX_TRIANGLES     = 100000;
    static const uint32_t MAX_VERTICES      = MAX_TRIANGLES * 3;
    static const uint32_t MAX_VERTICES_SIZE = MAX_VERTICES * sizeof(VertexData);
    static const uint32_t MAX_MESHLETS      = 10000;
    static const uint32_t MAX_MESHLET_SIZE  = MAX_MESHLETS * sizeof(MeshletData);
    static const uint32_t MAX_MESHLET_DATA_SIZE =
        MAX_MESHLETS * (MESHLET_MAX_VERTICES + MESHLET_MAX_TRIANGLES * 3) * sizeof(uint32_t);
    static const uint32_t MAX_MESH_DRAWS                 = 2048;
    static const uint32_t MAX_MESH_DRAW_SIZE             = MAX_MESH_DRAWS * sizeof(MeshDraw);
    static const uint32_t MAX_INDIRECT_DRAW_COMMAND_SIZE = MAX_MESH_DRAWS * sizeof(uint32_t) * 3;

    SceneData m_sceneData;

    Ref<IBuffer> m_vertexStorageBuffer    = nullptr;
    Ref<IBuffer> m_meshletBuffer          = nullptr;
    Ref<IBuffer> m_meshletDataBuffer      = nullptr;
    Ref<IBuffer> m_meshDrawBuffer         = nullptr;
    Ref<IBuffer> m_meshTaskIndirectBuffer = nullptr;

    std::vector<Ref<IRenderPass>> m_renderPasses;
    Ref<IShaderResourceBinding>   m_shaderResourceBinding = nullptr;

    std::unordered_map<uint64_t, Ref<IFrameBuffer>> m_frameBuffers;
};

} // namespace Yogi