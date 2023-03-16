#pragma once

#include <engine.h>

class Sandbox2D : public Yogi::Layer
{
public:
    Sandbox2D();
    ~Sandbox2D() = default;

    void on_attach() override;
    void on_detach() override;
    void on_update(Yogi::Timestep ts) override;
    void on_event(Yogi::Event& event) override;

private:
    Yogi::Ref<Yogi::Scene> m_scene;

    Yogi::Ref<Yogi::Texture2D> m_checkerboard_texture;

    glm::vec4 m_square_color = { 0.2f, 0.3f, 0.8f, 1.0f };
};