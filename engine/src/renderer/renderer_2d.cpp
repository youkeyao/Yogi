#include "renderer/renderer_command.h"
#include "renderer/renderer_2d.h"
#include "renderer/shader.h"
#include "renderer/vertex_array.h"

#include <glm/gtc/matrix_transform.hpp>

namespace hazel {

    struct Renderer2DStorage
    {
        Ref<VertexArray> quad_vertex_array;
        Ref<Shader> texture_shader;
        Ref<Texture2D> white_texture;
    };

    static Scope<Renderer2DStorage> s_data;

    void Renderer2D::init()
    {
        s_data = std::make_unique<Renderer2DStorage>();
        s_data->quad_vertex_array = VertexArray::create();

        float square_vertices[] = {
            -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
             0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
             0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
            -0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
        };

        Ref<VertexBuffer> square_vb = VertexBuffer::create(square_vertices, sizeof(square_vertices));
        square_vb->set_layout({
            { ShaderDataType::Float3, "a_Position" },
            { ShaderDataType::Float2, "a_TexCoord" },
        });
        s_data->quad_vertex_array->add_vertex_buffer(square_vb);

        uint32_t square_indices[] = { 0, 1, 2, 2, 3, 0 };
        Ref<IndexBuffer> square_ib = IndexBuffer::create(square_indices, sizeof(square_indices) / sizeof(uint32_t));
        s_data->quad_vertex_array->set_index_buffer(square_ib);

        s_data->white_texture = Texture2D::create(1, 1);
        uint32_t white_data = 0xffffffff;
        s_data->white_texture->set_data(&white_data, sizeof(uint32_t));

        s_data->texture_shader = hazel::Shader::create("../sandbox/assets/shaders/Texture.glsl");
        s_data->texture_shader->bind();
        s_data->texture_shader->set_int("u_texture", 0);
    }

    void Renderer2D::shutdown() { s_data.reset(); }

    void Renderer2D::begin_scene(const OrthographicCamera& camera)
    {
        s_data->texture_shader->bind();
        s_data->texture_shader->set_mat4("u_view_projection", camera.get_view_projection_matrix());
    }

    void Renderer2D::end_scene() {}

    void Renderer2D::draw_quad(const glm::vec3& position, const float rotation, const glm::vec2& size, const glm::vec4& color)
    {
        s_data->texture_shader->bind();
        s_data->texture_shader->set_float4("u_color", color);
        s_data->texture_shader->set_float("u_texture_scale", 1.0f);
        s_data->white_texture->bind();

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});
        s_data->texture_shader->set_mat4("u_transform", transform);

        s_data->quad_vertex_array->bind();
        RendererCommand::draw_indexed(s_data->quad_vertex_array);
    }

    void Renderer2D::draw_quad(const glm::vec3& position, const float rotation, const glm::vec2& size, const Ref<Texture2D>& texture, const float texture_scale, const glm::vec4& color)
    {
        s_data->texture_shader->bind();
        s_data->texture_shader->set_float4("u_color", color);
        s_data->texture_shader->set_float("u_texture_scale", texture_scale);
        texture->bind();

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});
        s_data->texture_shader->set_mat4("u_transform", transform);

        s_data->quad_vertex_array->bind();
        RendererCommand::draw_indexed(s_data->quad_vertex_array);
    }

}