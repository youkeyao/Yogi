#pragma once

#include <engine.h>

class Sandbox2D : public hazel::Layer
{
public:
    Sandbox2D();
    ~Sandbox2D() = default;

    void on_attach() override;
    void on_detach() override;
    void on_update(hazel::TimeStep ts) override;
    void on_imgui_render() override;
    void on_event(hazel::Event& event) override;

private:
    hazel::OrthographicCameraController m_camera_controller;

    hazel::Ref<hazel::VertexArray> m_square_va;
    hazel::Ref<hazel::Shader> m_flat_color_shader;
    hazel::Ref<hazel::Texture2D> m_checkerboard_texture;

    glm::vec4 m_square_color = { 0.2f, 0.3f, 0.8f, 1.0f };
};