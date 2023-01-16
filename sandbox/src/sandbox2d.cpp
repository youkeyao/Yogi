#include <imgui.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "sandbox2d.h"

Sandbox2D::Sandbox2D() : Layer("Sandbox 2D"), m_camera_controller(1280.0f / 720.0f) {}

void Sandbox2D::on_attach()
{
    m_square_va = hazel::VertexArray::create();
    float square_vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.5f,  0.5f, 0.0f,
        -0.5f,  0.5f, 0.0f,
    };
    
    hazel::Ref<hazel::VertexBuffer> square_vb;
    square_vb = hazel::VertexBuffer::create(square_vertices, sizeof(square_vertices));
    square_vb->set_layout({
        { hazel::ShaderDataType::Float3, "a_Position" },
    });
    m_square_va->add_vertex_buffer(square_vb);

    uint32_t square_indices[] = {
        0, 1, 2, 2, 3, 0
    };
    hazel::Ref<hazel::IndexBuffer> square_ib;
    square_ib = hazel::IndexBuffer::create(square_indices, 6);
    m_square_va->set_index_buffer(square_ib);

    m_flat_color_shader = hazel::Shader::create("../sandbox/assets/shaders/FlatColor.glsl");

    m_checkerboard_texture = hazel::Texture2D::create("../sandbox/assets/textures/checkerboard.png");
}

void Sandbox2D::on_detach() {}

void Sandbox2D::on_update(hazel::TimeStep ts)
{
    m_camera_controller.on_update(ts);

    hazel::RendererCommand::set_clear_color({ 0.1f, 0.1f, 0.1f, 1.0f });
    hazel::RendererCommand::clear();

    hazel::Renderer2D::begin_scene(m_camera_controller.get_camera());
    
    m_flat_color_shader->bind();
    m_flat_color_shader->set_float4("u_color", m_square_color);

    hazel::Renderer::submit(m_flat_color_shader, m_square_va, glm::scale(glm::mat4(1.0f), glm::vec3(1.5f)));

    hazel::Renderer2D::draw_quad({0.0f, 0.0f}, {1.0f, 1.0f}, {0.8f, 0.2f, 0.3f, 1.0f});

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