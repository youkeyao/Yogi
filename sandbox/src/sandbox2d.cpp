#include "sandbox2d.h"
#include <imgui.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Sandbox2D::Sandbox2D() : Layer("Sandbox 2D"), m_camera_controller(1280.0f / 720.0f) {}

void Sandbox2D::on_attach()
{
    HZ_PROFILE_FUNCTION();

    m_checkerboard_texture = hazel::Texture2D::create("../sandbox/assets/textures/checkerboard.png");
}

void Sandbox2D::on_detach()
{
    HZ_PROFILE_FUNCTION();
}

void Sandbox2D::on_update(hazel::TimeStep ts)
{
    HZ_PROFILE_FUNCTION();

    m_camera_controller.on_update(ts);

    hazel::Renderer2D::reset_stats();
    {
        HZ_PROFILE_SCOPE("Render prep");
        hazel::RenderCommand::set_clear_color({ 0.1f, 0.1f, 0.1f, 1.0f });
        hazel::RenderCommand::clear();
    }

    {
        HZ_PROFILE_SCOPE("Render draw");
        hazel::Renderer2D::begin_scene(m_camera_controller.get_camera());
        hazel::Renderer2D::draw_quad({-1.0f, 0.0f}, 45.0f, {0.8f, 0.8f}, {0.8f, 0.2f, 0.3f, 1.0f});
        hazel::Renderer2D::draw_quad({0.5f, -0.5f}, {0.5f, 0.75f}, {0.2f, 0.3f, 0.8f, 1.0f});
        hazel::Renderer2D::draw_quad({-0.5f, 0.5f}, {0.5f, 0.75f}, {0.2f, 0.8f, 0.3f, 1.0f});
        hazel::Renderer2D::draw_quad({ 0.0f, 0.0f, -0.1f }, { 20.0f, 20.0f }, m_checkerboard_texture, 10.0f, m_square_color);
        hazel::Renderer2D::draw_quad({ 0.0f, 0.0f, 0.1f }, 45.0f, { 1.0f, 1.0f }, m_checkerboard_texture, 5.0f);

        for (float y = -5.0f; y < 5.0f; y += 0.5f) {
            for (float x = -5.0f; x < 5.0f; x += 0.5f) {
                hazel::Renderer2D::draw_quad({x, y}, {0.45f, 0.45f}, {(x + 5.0f) / 10.0f, 0.4f, (y + 5.0f) / 10.0f, 0.7f});
            }
        }
        hazel::Renderer2D::end_scene();
    }
}

void Sandbox2D::on_imgui_render()
{
    HZ_PROFILE_FUNCTION();
    
    hazel::Renderer2D::Statistics stats = hazel::Renderer2D::get_stats();
    ImGui::Begin("Settings");
    ImGui::Text("Renderer2D Stats:");
    ImGui::Text("Draw Calls: %d", stats.draw_calls);
    ImGui::Text("Quads: %d", stats.quad_count);
    ImGui::Text("Vertices: %d", stats.get_total_vertex_count());
    ImGui::Text("Indices: %d", stats.get_total_index_count());
    ImGui::ColorEdit4("Square color", glm::value_ptr(m_square_color));
    ImGui::End();
}

void Sandbox2D::on_event(hazel::Event& e)
{
    m_camera_controller.on_event(e);
}