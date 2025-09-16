#pragma once

#include <Yogi.h>

class Sandbox2D : public Yogi::Layer
{
public:
    Sandbox2D();
    virtual ~Sandbox2D() = default;

    void OnUpdate(Yogi::Timestep ts) override;
    void OnEvent(Yogi::Event& event) override;

private:
    Yogi::Handle<Yogi::World> m_world = nullptr;
    Yogi::Entity              m_box   = Yogi::Entity::Null();

    Yogi::Matrix4 m_transform = Yogi::Matrix4::Identity();

    Yogi::Handle<Yogi::IRenderPass>            m_renderPass            = nullptr;
    Yogi::Handle<Yogi::IPipeline>              m_pipeline              = nullptr;
    Yogi::Handle<Yogi::ICommandBuffer>         m_commandBuffer         = nullptr;
    Yogi::Handle<Yogi::IShaderResourceBinding> m_shaderResourceBinding = nullptr;
};