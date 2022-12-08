#include <engine.h>

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
        std::shared_ptr<hazel::VertexBuffer> m_vertex_buffer;
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
        std::shared_ptr<hazel::IndexBuffer> m_index_buffer;
        m_index_buffer.reset(hazel::IndexBuffer::create(indices, 3));
        m_vertex_array->set_index_buffer(m_index_buffer);

        float square_vertices[] = {
            -0.75f, -0.75f, 0.0f,
             0.75f, -0.75f, 0.0f,
             0.75f,  0.75f, 0.0f,
            -0.75f,  0.75f, 0.0f,
        };
        m_square_va.reset(hazel::VertexArray::create());
        std::shared_ptr<hazel::VertexBuffer> square_vb;
        square_vb.reset(hazel::VertexBuffer::create(square_vertices, sizeof(square_vertices)));
        square_vb->set_layout({
            { hazel::ShaderDataType::Float3, "a_Position" },
        });
        m_square_va->add_vertex_buffer(square_vb);

        uint32_t square_indices[] = {
            0, 1, 2, 2, 3, 0
        };
        std::shared_ptr<hazel::IndexBuffer> square_ib;
        square_ib.reset(hazel::IndexBuffer::create(square_indices, 6));
        m_square_va->set_index_buffer(square_ib);

        std::string vertexSrc = R"(
            #version 330 core
            layout(location = 0) in vec3 a_Position;
            layout(location = 1) in vec4 a_Color;

            uniform mat4 u_view_projection;

            out vec3 v_Position;
            out vec4 v_Color;
            void main()
            {
                v_Position = a_Position;
                v_Color = a_Color;
                gl_Position = u_view_projection * vec4(a_Position, 1.0);
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

        m_shader.reset(new hazel::Shader(vertexSrc, fragmentSrc));

        std::string blueVertexSrc = R"(
            #version 330 core
            layout(location = 0) in vec3 a_Position;

            uniform mat4 u_view_projection;

            out vec3 v_Position;
            void main()
            {
                v_Position = a_Position;
                gl_Position = u_view_projection * vec4(a_Position, 1.0);
            }
        )";

        std::string blueFragmentSrc = R"(
            #version 330 core
            layout(location = 0) out vec4 color;
            
            in vec3 v_Position;
            void main()
            {
                color = vec4(0.2, 0.3, 0.8, 1.0);
            }
        )";

        m_blue_shader.reset(new hazel::Shader(blueVertexSrc, blueFragmentSrc));
    }

    void on_update() override
    {
        if (hazel::Input::is_key_pressed(HZ_KEY_LEFT)) {
            m_camera.set_position(m_camera.get_position() - glm::vec3(m_camera_move_speed, 0, 0));
        }
        if (hazel::Input::is_key_pressed(HZ_KEY_RIGHT)) {
            m_camera.set_position(m_camera.get_position() + glm::vec3(m_camera_move_speed, 0, 0));
        }
        if (hazel::Input::is_key_pressed(HZ_KEY_UP)) {
            m_camera.set_position(m_camera.get_position() + glm::vec3(0, m_camera_move_speed, 0));
        }
        if (hazel::Input::is_key_pressed(HZ_KEY_DOWN)) {
            m_camera.set_position(m_camera.get_position() - glm::vec3(0, m_camera_move_speed, 0));
        }
        if (hazel::Input::is_key_pressed(HZ_KEY_A)) {
            m_camera.set_rotation(m_camera.get_rotation() + m_camera_rotate_speed);
        }
        if (hazel::Input::is_key_pressed(HZ_KEY_D)) {
            m_camera.set_rotation(m_camera.get_rotation() - m_camera_rotate_speed);
        }

        hazel::RendererCommand::set_clear_color({ 0.1f, 0.1f, 0.1f, 1.0f });
        hazel::RendererCommand::clear();

        hazel::Renderer::begin_scene(m_camera);

        hazel::Renderer::submit(m_blue_shader, m_square_va);
        hazel::Renderer::submit(m_shader, m_vertex_array);

        hazel::Renderer::end_scene();
    }

    void on_event(hazel::Event& event) override
    {
    }

private:
    std::shared_ptr<hazel::Shader> m_shader;
    std::shared_ptr<hazel::VertexArray> m_vertex_array;

    std::shared_ptr<hazel::Shader> m_blue_shader;
    std::shared_ptr<hazel::VertexArray> m_square_va;

    hazel::OrthographicCamera m_camera;
    float m_camera_move_speed = 0.1f;
    float m_camera_rotate_speed = 2.0f;
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