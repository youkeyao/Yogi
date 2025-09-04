#pragma once

#include <Yogi.h>

class Sandbox2D : public Yogi::Layer
{
public:
    Sandbox2D();
    virtual ~Sandbox2D() = default;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(Yogi::Timestep ts) override;
    void OnEvent(Yogi::Event& event) override;

private:
    // Yogi::Scope<Yogi::Scene> m_scene;
    // Yogi::Entity           checker;

    // glm::vec4 m_square_color = {0.2f, 0.3f, 0.8f, 1.0f};
    Yogi::Scope<Yogi::IBuffer> m_vertexBuffer;
    Yogi::Scope<Yogi::IBuffer> m_indexBuffer;
    Yogi::Scope<Yogi::IBuffer> m_uniformBuffer;

    Yogi::Matrix4 m_transform = Yogi::Matrix4::Identity();

    Yogi::Scope<Yogi::IRenderPass>            m_renderPass;
    Yogi::Scope<Yogi::IPipeline>              m_pipeline;
    Yogi::Scope<Yogi::ICommandBuffer>         m_commandBuffer;
    Yogi::Scope<Yogi::IShaderResourceBinding> m_shaderResourceBinding;
};