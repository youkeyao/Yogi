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
    Yogi::Scope<Yogi::World> m_world;
    // Yogi::Entity           checker;

    // Yogi::Scope<Yogi::IBuffer> m_vertexBuffer;
    // Yogi::Scope<Yogi::IBuffer> m_indexBuffer;
    // Yogi::Scope<Yogi::IBuffer> m_uniformBuffer;

    Yogi::Matrix4 m_transform = Yogi::Matrix4::Identity();

    Yogi::AssetHandle<Yogi::IRenderPass> m_renderPass;
    // Yogi::Scope<Yogi::IRenderPass>            m_renderPass;
    Yogi::Scope<Yogi::IPipeline>              m_pipeline;
    // Yogi::Scope<Yogi::ICommandBuffer>         m_commandBuffer;
    Yogi::AssetHandle<Yogi::IShaderResourceBinding> m_shaderResourceBinding;
    // Yogi::Scope<Yogi::IShaderResourceBinding> m_shaderResourceBinding;
};