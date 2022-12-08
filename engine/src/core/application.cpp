#include "core/application.h"
#include "core/input.h"
#include "renderer/renderer.h"
#include "renderer/orthographic_camera.h"

namespace hazel {

    #define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)

    Application* Application::ms_instance = nullptr;

    Application::Application() : m_camera(-1.6f, 1.6f, -0.9f, 0.9f)
    {
        HZ_CORE_ASSERT(!ms_instance, "Application already exists!");
        ms_instance = this;

        m_window = std::unique_ptr<Window>(Window::create());
        m_window->set_event_callback(BIND_EVENT_FN(Application::on_event));

        m_imgui_layer = new ImGuiLayer();
        push_overlay(m_imgui_layer);

        m_vertex_array.reset(VertexArray::create());

        float vertices[] = {
            -0.5f, -0.5f, 0.0f, 0.8f, 0.2f, 0.9f, 1.0f,
             0.5f, -0.5f, 0.0f, 0.2f, 0.3f, 0.8f, 1.0f,
             0.0f,  0.5f, 0.0f, 0.8f, 0.8f, 0.2f, 1.0f,
        };
        std::shared_ptr<VertexBuffer> m_vertex_buffer;
        m_vertex_buffer.reset(VertexBuffer::create(vertices, sizeof(vertices)));
        BufferLayout layout = {
            { ShaderDataType::Float3, "a_Position" },
            { ShaderDataType::Float4, "a_Color" },
        };
        m_vertex_buffer->set_layout(layout);
        m_vertex_array->add_vertex_buffer(m_vertex_buffer);

        uint32_t indices[] = {
            0, 1, 2,
        };
        std::shared_ptr<IndexBuffer> m_index_buffer;
        m_index_buffer.reset(IndexBuffer::create(indices, 3));
        m_vertex_array->set_index_buffer(m_index_buffer);

        float square_vertices[] = {
            -0.75f, -0.75f, 0.0f,
             0.75f, -0.75f, 0.0f,
             0.75f,  0.75f, 0.0f,
            -0.75f,  0.75f, 0.0f,
        };
        m_square_va.reset(VertexArray::create());
        std::shared_ptr<VertexBuffer> square_vb;
        square_vb.reset(VertexBuffer::create(square_vertices, sizeof(square_vertices)));
        square_vb->set_layout({
            { ShaderDataType::Float3, "a_Position" },
        });
        m_square_va->add_vertex_buffer(square_vb);

        uint32_t square_indices[] = {
            0, 1, 2, 2, 3, 0
        };
        std::shared_ptr<IndexBuffer> square_ib;
        square_ib.reset(IndexBuffer::create(square_indices, 6));
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

        m_shader.reset(new Shader(vertexSrc, fragmentSrc));

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

        m_blue_shader.reset(new Shader(blueVertexSrc, blueFragmentSrc));
    }

    Application::~Application()
    {

    }

    void Application::push_layer(Layer* layer)
    {
        m_layerstack.push_layer(layer);
        layer->on_attach();
    }

    void Application::push_overlay(Layer* layer)
    {
        m_layerstack.push_overlay(layer);
        layer->on_attach();
    }

    void Application::on_event(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::on_window_close));

        for (auto it = m_layerstack.end(); it != m_layerstack.begin();) {
            (*--it)->on_event(e);
            if (e.m_handled) {
                break;
            }
        }
    }

    void Application::run()
    {
        while (m_running) {
            RendererCommand::set_clear_color({ 0.1f, 0.1f, 0.1f, 1.0f });
            RendererCommand::clear();

            m_camera.set_position({ 0.5f, 0.5f, 0.0f });
            m_camera.set_rotation(45.0f);

            Renderer::begin_scene(m_camera);

            Renderer::submit(m_blue_shader, m_square_va);
            Renderer::submit(m_shader, m_vertex_array);

            Renderer::end_scene();

            for (Layer* layer : m_layerstack) {
                layer->on_update();
            }
            m_imgui_layer->begin();
            for (Layer* layer : m_layerstack) {
                layer->on_imgui_render();
            }
            m_imgui_layer->end();

            m_window->on_update();
        }
    }

    bool Application::on_window_close(WindowCloseEvent& e)
    {
        m_running = false;
        return true;
    }

}