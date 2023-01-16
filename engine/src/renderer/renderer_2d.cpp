#include "renderer/renderer_command.h"
#include "renderer/renderer_2d.h"
#include "renderer/shader.h"
#include "renderer/vertex_array.h"

#include <glm/gtc/matrix_transform.hpp>

namespace hazel {

    struct Renderer2DStorage
    {
        Ref<VertexArray> quad_vertex_array;
        Ref<Shader> flat_color_shader;
        Ref<Texture> white_texture;
    };

    static Scope<Renderer2DStorage> s_data;

    void Renderer2D::init()
    {
        s_data = std::make_unique<Renderer2DStorage>();
        s_data->quad_vertex_array = VertexArray::create();

        float square_vertices[] = {
            -0.5f, -0.5f, 0.0f,
             0.5f, -0.5f, 0.0f,
             0.5f,  0.5f, 0.0f,
            -0.5f,  0.5f, 0.0f,
        };

        Ref<VertexBuffer> square_vb = VertexBuffer::create(square_vertices, sizeof(square_vertices));
        square_vb->set_layout({
            { ShaderDataType::Float3, "a_Position" },
        });
        s_data->quad_vertex_array->add_vertex_buffer(square_vb);

        uint32_t square_indices[] = { 0, 1, 2, 2, 3, 0 };
        Ref<IndexBuffer> square_ib = IndexBuffer::create(square_indices, sizeof(square_indices) / sizeof(uint32_t));
        s_data->quad_vertex_array->set_index_buffer(square_ib);

        s_data->flat_color_shader = hazel::Shader::create("../sandbox/assets/shaders/FlatColor.glsl");
    }

    void Renderer2D::shutdown() { s_data.reset(); }

    void Renderer2D::begin_scene(const OrthographicCamera& camera)
    {
        s_data->flat_color_shader->bind();
        s_data->flat_color_shader->set_mat4("u_view_projection", camera.get_view_projection_matrix());
    }

    void Renderer2D::end_scene() {}

    void Renderer2D::draw_quad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
    {
        draw_quad({ position.x, position.y, 0.0f }, size, color);
    }

    void Renderer2D::draw_quad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
    {
        s_data->flat_color_shader->bind();
        s_data->flat_color_shader->set_float4("u_color", color);

        auto transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});
        s_data->flat_color_shader->set_mat4("u_transform", transform);

        s_data->quad_vertex_array->bind();
        RendererCommand::draw_indexed(s_data->quad_vertex_array);
    }

    void Renderer2D::draw_quad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture)
    {
        draw_quad({ position.x, position.y, 1.0f }, size, texture);
    }

    void Renderer2D::draw_quad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture)
    {
        s_data->flat_color_shader->bind();
        s_data->flat_color_shader->set_float4("u_color", glm::vec4(1.0f));

        auto transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});
        s_data->flat_color_shader->set_mat4("u_transform", transform);

        texture->bind();

        s_data->quad_vertex_array->bind();
        RendererCommand::draw_indexed(s_data->quad_vertex_array);
    }

}