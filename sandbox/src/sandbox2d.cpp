#include <imgui.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "sandbox2d.h"

Sandbox2D::Sandbox2D() : Layer("Sandbox 2D"), m_camera_controller(1280.0f / 720.0f) {}

void Sandbox2D::on_attach()
{
    m_checkerboard_texture = hazel::Texture2D::create("../sandbox/assets/textures/checkerboard.png");
}

void Sandbox2D::on_detach() {}

void Sandbox2D::on_update(hazel::TimeStep ts)
{
    m_camera_controller.on_update(ts);

    hazel::RendererCommand::set_clear_color({ 0.1f, 0.1f, 0.1f, 1.0f });
    hazel::RendererCommand::clear();

    hazel::Renderer2D::begin_scene(m_camera_controller.get_camera());
    hazel::Renderer2D::draw_quad({-1.0f, 0.0f}, {0.8f, 0.8f}, {0.8f, 0.2f, 0.3f, 1.0f});
    hazel::Renderer2D::draw_quad({0.5f, -0.5f}, {0.5f, 0.75f}, {0.2f, 0.3f, 0.8f, 1.0f});
    hazel::Renderer2D::draw_quad({ 0.0f, 0.0f, -0.1f }, { 10.0f, 10.0f }, m_checkerboard_texture, 10.0f);
    hazel::Renderer2D::end_scene();
}

void Sandbox2D::on_imgui_render()
{
    ImGui::Begin("Settings");
    ImGui::ColorEdit4("Square color", glm::value_ptr(m_square_color));
    ImGui::End();
}

void Sandbox2D::on_event(hazel::Event& e)
{
    m_camera_controller.on_event(e);
}