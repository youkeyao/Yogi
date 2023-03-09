#include "sandbox2d.h"
#include <glm/gtc/type_ptr.hpp>

Sandbox2D::Sandbox2D() : Layer("Sandbox 2D") {}

void Sandbox2D::on_attach()
{
    YG_PROFILE_FUNCTION();

    // m_checkerboard_texture = Yogi::Texture2D::create("../sandbox/assets/textures/checkerboard.png");

    m_scene = Yogi::CreateRef<Yogi::Scene>();

    // m_scene->add_system<Yogi::RenderSystem>();

    // for (int32_t i = 0; i < 10000; i ++) {
    //     Yogi::Entity e = m_scene->create_entity();
    //     e.add_component<Yogi::TransformComponent>();
    //     e.add_component<Yogi::SpriteRendererComponent>();
    // }
}

void Sandbox2D::on_detach()
{
    YG_PROFILE_FUNCTION();
}

void Sandbox2D::on_update(Yogi::Timestep ts)
{
    YG_PROFILE_FUNCTION();
    // YG_CORE_INFO("{0}", 1.0f / ts);

    // m_camera_controller.on_update(ts);

    // Yogi::Renderer2D::reset_stats();
    {
        // YG_PROFILE_SCOPE("Render prep");
        // Yogi::RenderCommand::set_clear_color({ 0.1f, 0.1f, 0.1f, 1.0f });
        // Yogi::RenderCommand::clear();
    }

    {
        YG_PROFILE_SCOPE("Render draw");
        // Yogi::Renderer2D::begin_scene(m_camera_controller.get_camera());
        // Yogi::Renderer2D::draw_quad({-1.0f, 0.0f}, glm::radians(45.0f), {0.8f, 0.8f}, {0.8f, 0.2f, 0.3f, 1.0f});
        // Yogi::Renderer2D::draw_quad({0.5f, -0.5f}, {0.5f, 0.75f}, {0.2f, 0.3f, 0.8f, 1.0f});
        // Yogi::Renderer2D::draw_quad({-0.5f, 0.5f}, {0.5f, 0.75f}, {0.2f, 0.8f, 0.3f, 1.0f});
        // Yogi::Renderer2D::draw_quad({ 0.0f, 0.0f, -0.1f }, { 20.0f, 20.0f }, m_checkerboard_texture, {{0.0f, 0.0f}, {10.0f, 10.0f}}, m_square_color);
        // Yogi::Renderer2D::draw_quad({ 0.0f, 0.0f, 0.1f }, glm::radians(45.0f), { 1.0f, 1.0f }, m_checkerboard_texture, {{0.0f, 0.0f}, {5.0f, 5.0f}});

        // for (float y = -5.0f; y < 5.0f; y += 0.5f) {
        //     for (float x = -5.0f; x < 5.0f; x += 0.5f) {
        //         Yogi::Renderer2D::draw_quad({x, y}, {0.45f, 0.45f}, {(x + 5.0f) / 10.0f, 0.4f, (y + 5.0f) / 10.0f, 0.7f});
        //     }
        // }
        m_scene->on_update(ts);
        // Yogi::Renderer2D::end_scene();
    }

    // if (Yogi::Input::is_mouse_button_pressed(YG_MOUSE_BUTTON_LEFT)) {
    //     auto [x, y] = Yogi::Input::get_mouse_position();
    //     auto width = Yogi::Application::get().get_window().get_width();
    //     auto height = Yogi::Application::get().get_window().get_height();

    //     glm::vec2 bounds = { 2 * m_camera_controller.zoom_level() * 1280.0f / 720.0f, 2 * m_camera_controller.zoom_level() };
    //     auto pos = m_camera_controller.get_camera().get_position();
    //     x = (x / width) * bounds.x - bounds.x * 0.5f;
    //     y = bounds.y * 0.5f - (y / height) * bounds.y;
    //     m_particle_props.position = { x + pos.x, y + pos.y };
    //     for (int i = 0; i < 5; i++)
    //         m_particle_system.emit(m_particle_props);
    // }

    // m_particle_system.on_update(ts);
    // m_particle_system.on_render(m_camera_controller.get_camera());
}

void Sandbox2D::on_event(Yogi::Event& e)
{
    // m_camera_controller.on_event(e);
}