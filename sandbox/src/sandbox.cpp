#include <engine.h>
#include <glm/gtc/matrix_transform.hpp>
#include "platform/opengl/opengl_shader.h"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

class ExampleLayer : public hazel::Layer
{
public:
    ExampleLayer() : Layer("example"), m_camera(-1.6f, 1.6f, -0.9f, 0.9f)
    {
        m_vertex_array.reset(hazel::VertexArray::create());

        float vertices[] = {
            -0.5f, -0.5f, 0.0f, 0.8f, 0.2f, 0.9f, 1.0f,
             0.5f, -0.5f, 0.0f, 0.2f, 0.3f, 0.8f, 1.0f,
             0.0f,  0.5f, 0.0f, 0.8f, 0.8f, 0.2f, 1.0f,
        };
        hazel::Ref<hazel::VertexBuffer> m_vertex_buffer;
        m_vertex_buffer.reset(hazel::VertexBuffer::create(vertices, sizeof(vertices)));
        hazel::BufferLayout layout = {
            { hazel::ShaderDataType::Float3, "a_Position" },
            { hazel::ShaderDataType::Float4, "a_Color" },
        };
        m_vertex_buffer->set_layout(layout);
        m_vertex_array->add_vertex_buffer(m_vertex_buffer);

        uint32_t indices[] = {
            0, 1, 2,
        };
        hazel::Ref<hazel::IndexBuffer> m_index_buffer;
        m_index_buffer.reset(hazel::IndexBuffer::create(indices, 3));
        m_vertex_array->set_index_buffer(m_index_buffer);

        float square_vertices[] = {
            -0.75f, -0.75f, 0.0f, 0.0f, 0.0f,
             0.75f, -0.75f, 0.0f, 1.0f, 0.0f,
             0.75f,  0.75f, 0.0f, 1.0f, 1.0f,
            -0.75f,  0.75f, 0.0f, 0.0f, 1.0f,
        };
        m_square_va.reset(hazel::VertexArray::create());
        hazel::Ref<hazel::VertexBuffer> square_vb;
        square_vb.reset(hazel::VertexBuffer::create(square_vertices, sizeof(square_vertices)));
        square_vb->set_layout({
            { hazel::ShaderDataType::Float3, "a_Position" },
            { hazel::ShaderDataType::Float2, "a_TexCoord" },
        });
        m_square_va->add_vertex_buffer(square_vb);

        uint32_t square_indices[] = {
            0, 1, 2, 2, 3, 0
        };
        hazel::Ref<hazel::IndexBuffer> square_ib;
        square_ib.reset(hazel::IndexBuffer::create(square_indices, 6));
        m_square_va->set_index_buffer(square_ib);

        std::string vertexSrc = R"(
            #version 330 core
            layout(location = 0) in vec3 a_Position;
            layout(location = 1) in vec4 a_Color;

            uniform mat4 u_view_projection;
            uniform mat4 u_transform;

            out vec3 v_Position;
            out vec4 v_Color;
            void main()
            {
                v_Position = a_Position;
                v_Color = a_Color;
                gl_Position = u_view_projection * u_transform * vec4(a_Position, 1.0);
            }
        )";

        std::string fragmentSrc = R"(
            #version 330 core
            layout(location = 0) out vec4 color;
            
            in vec3 v_Position;
            in vec4 v_Color;
            void main()
            {
                color = vec4(0.8, 0.2, 0.3, 1.0);
                color = v_Color;
            }
        )";

        m_shader = hazel::Shader::create("VertexPosColor", vertexSrc, fragmentSrc);

        std::string squareVertexSrc = R"(
            #version 330 core
            layout(location = 0) in vec3 a_Position;

            uniform mat4 u_view_projection;
            uniform mat4 u_transform;

            out vec3 v_Position;
            void main()
            {
                v_Position = a_Position;
                gl_Position = u_view_projection * u_transform * vec4(a_Position, 1.0);
            }
        )";

        std::string squareFragmentSrc = R"(
            #version 330 core
            layout(location = 0) out vec4 color;

            uniform vec3 u_color;
            
            in vec3 v_Position;
            void main()
            {
                color = vec4(u_color, 1.0);
            }
        )";

        m_square_shader = hazel::Shader::create("FlatColor", squareVertexSrc, squareFragmentSrc);

        auto texture_shader = m_shader_library.load("../sandbox/assets/shaders/Texture.glsl");

        m_texture = hazel::Texture2D::create("../sandbox/assets/textures/checkerboard.png");
        m_logo_texture = hazel::Texture2D::create("../sandbox/assets/textures/cherno_logo.png");

        std::dynamic_pointer_cast<hazel::OpenGLShader>(texture_shader)->bind();
        std::dynamic_pointer_cast<hazel::OpenGLShader>(texture_shader)->set_int("u_texture", 0);
    }

    void on_update(hazel::TimeStep ts) override
    {        
        if (hazel::Input::is_key_pressed(HZ_KEY_LEFT)) {
            m_camera.set_position(m_camera.get_position() - glm::vec3(m_camera_move_speed * ts, 0, 0));
        }
        if (hazel::Input::is_key_pressed(HZ_KEY_RIGHT)) {
            m_camera.set_position(m_camera.get_position() + glm::vec3(m_camera_move_speed * ts, 0, 0));
        }
        if (hazel::Input::is_key_pressed(HZ_KEY_UP)) {
            m_camera.set_position(m_camera.get_position() + glm::vec3(0, m_camera_move_speed * ts, 0));
        }
        if (hazel::Input::is_key_pressed(HZ_KEY_DOWN)) {
            m_camera.set_position(m_camera.get_position() - glm::vec3(0, m_camera_move_speed * ts, 0));
        }
        if (hazel::Input::is_key_pressed(HZ_KEY_A)) {
            m_camera.set_rotation(m_camera.get_rotation() + m_camera_rotate_speed * ts);
        }
        if (hazel::Input::is_key_pressed(HZ_KEY_D)) {
            m_camera.set_rotation(m_camera.get_rotation() - m_camera_rotate_speed * ts);
        }

        hazel::RendererCommand::set_clear_color({ 0.1f, 0.1f, 0.1f, 1.0f });
        hazel::RendererCommand::clear();

        hazel::Renderer::begin_scene(m_camera);

        glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.06f));

        std::dynamic_pointer_cast<hazel::OpenGLShader>(m_square_shader)->bind();
        std::dynamic_pointer_cast<hazel::OpenGLShader>(m_square_shader)->set_float3("u_color", m_square_color);

        for (int y = 0; y < 20; y ++) {
            for (int x = 0; x < 20; x ++) {
                glm::vec3 pos(x * 0.11f, y * 0.11f, 0.0f);
                glm::mat4 transform = glm::translate(glm::mat4(1.0f), pos) * scale;
                hazel::Renderer::submit(m_square_shader, m_square_va, transform);
            }
        }

        auto texture_shader = m_shader_library.get("Texture");

        m_texture->bind();
        hazel::Renderer::submit(texture_shader, m_square_va);
        m_logo_texture->bind();
        hazel::Renderer::submit(texture_shader, m_square_va);
        
        // hazel::Renderer::submit(m_shader, m_vertex_array);

        hazel::Renderer::end_scene();
    }

    void on_imgui_render()
    {
        ImGui::Begin("Settings");
        ImGui::ColorEdit3("Square Color", glm::value_ptr(m_square_color));
        ImGui::End();
    }

    void on_event(hazel::Event& event) override
    {
    }

private:
    hazel::ShaderLibrary m_shader_library;
    hazel::Ref<hazel::Shader> m_shader;
    hazel::Ref<hazel::VertexArray> m_vertex_array;

    hazel::Ref<hazel::Shader> m_square_shader;
    hazel::Ref<hazel::VertexArray> m_square_va;
    glm::vec3 m_square_color = { 0.2f, 0.3f, 0.8f };

    hazel::Ref<hazel::Texture2D> m_texture, m_logo_texture;

    hazel::OrthographicCamera m_camera;
    float m_camera_move_speed = 5.0f;
    float m_camera_rotate_speed = 180.0f;
};

class Sandbox : public hazel::Application
{
public:
    Sandbox()
    {
        push_layer(new ExampleLayer());
    }

    ~Sandbox()
    {

    }
};

hazel::Application* hazel::create_application()
{
    return new Sandbox();
}