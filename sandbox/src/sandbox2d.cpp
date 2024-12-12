#include "sandbox2d.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

Sandbox2D::Sandbox2D() : Layer("Sandbox 2D") {}

void Sandbox2D::on_attach()
{
    YG_PROFILE_FUNCTION();

    m_scene = Yogi::CreateRef<Yogi::Scene>();
    Yogi::Ref<Yogi::Mesh> quad = Yogi::MeshManager::get_mesh("quad");
    Yogi::Ref<Yogi::Material> mat1 = Yogi::MaterialManager::get_material("default");

    m_scene->add_system<Yogi::RenderSystem>();
    m_scene->add_system<Yogi::PhysicsSystem>();

    checker = m_scene->create_entity();
    checker.add_component<Yogi::TransformComponent>().transform = glm::translate(glm::mat4(1.0f), {0, -0.3, 0});
    checker.add_component<Yogi::MeshRendererComponent>(quad, mat1);

    Yogi::Entity e = m_scene->create_entity();
    e.add_component<Yogi::TransformComponent>().transform = glm::translate(glm::mat4(1.0f), {0, 0, 0.05});
    e.add_component<Yogi::MeshRendererComponent>(quad, mat1);

    e = m_scene->create_entity();
    e.add_component<Yogi::TransformComponent>().transform = glm::inverse(glm::lookAt(glm::vec3{2, 1, 2}, glm::vec3{0, 0, 0}, glm::vec3{0, 1, 0}));
    auto& camera = e.add_component<Yogi::CameraComponent>();
    camera.aspect_ratio = 1280.0f / 720.0f;
    camera.is_ortho = false;

    e = m_scene->create_entity();
    e.add_component<Yogi::TransformComponent>().transform = glm::translate(glm::mat4(1.0f), {0.4, 0, 0.1});
    e.add_component<Yogi::MeshRendererComponent>(quad, mat1);

    for (int32_t i = 0; i < 200; i ++) {
        for (int32_t j = 0; j < 200; j ++) {
            e = m_scene->create_entity();
            e.add_component<Yogi::TransformComponent>().transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.02 * i - 2, 0.02 * j - 2, 0.11)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.01, 0.01, 0.1));
            e.add_component<Yogi::MeshRendererComponent>(quad, mat1);
        }
    }

    e = m_scene->create_entity();
    e.add_component<Yogi::TransformComponent>().transform = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 1));
    e.add_component<Yogi::PointLightComponent>();

    e = m_scene->create_entity();
    e.add_component<Yogi::TransformComponent>().transform = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 2));
    e.add_component<Yogi::DirectionalLightComponent>();

    e = m_scene->create_entity();
    e.add_component<Yogi::TransformComponent>();
    e.add_component<Yogi::MeshRendererComponent>(Yogi::MeshManager::get_mesh("skybox"), Yogi::MaterialManager::get_material("skybox"));

    e = m_scene->create_entity();
    e.add_component<Yogi::TransformComponent>().transform = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0, 0.5, 1)), glm::vec3(0.5f));
    e.add_component<Yogi::MeshRendererComponent>(Yogi::MeshManager::get_mesh("cube"), mat1);
    e.add_component<Yogi::RigidBodyComponent>();

    e = m_scene->create_entity();
    e.add_component<Yogi::TransformComponent>().transform = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(0, -1.0f, 0)), glm::radians(-10.0f), glm::vec3(1, 0, 0)), glm::vec3(4.0f, 1.0f, 4.0f));
    e.add_component<Yogi::MeshRendererComponent>(Yogi::MeshManager::get_mesh("cube"), mat1);
    e.add_component<Yogi::RigidBodyComponent>().is_static = true;
}

void Sandbox2D::on_detach()
{
    YG_PROFILE_FUNCTION();
}

void Sandbox2D::on_update(Yogi::Timestep ts)
{
    YG_PROFILE_FUNCTION();
    YG_CORE_INFO("{0}", 1.0f / ts);

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
        auto& transform = checker.get_component<Yogi::TransformComponent>().transform;
        transform = glm::rotate(glm::mat4(1.0f), (float)ts, glm::vec3{0, 1, 0}) * (glm::mat4)transform;
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
    m_scene->on_event(e);
}