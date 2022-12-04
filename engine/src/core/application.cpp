#include "core/application.h"
#include "core/input.h"
#include <glad/glad.h>

namespace hazel {

    #define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)

    Application* Application::ms_instance = nullptr;

    Application::Application()
    {
        HZ_CORE_ASSERT(!ms_instance, "Application already exists!");
        ms_instance = this;

        m_window = std::unique_ptr<Window>(Window::create());
        m_window->set_event_callback(BIND_EVENT_FN(Application::on_event));

        m_imgui_layer = new ImGuiLayer();
        push_overlay(m_imgui_layer);

        glGenVertexArrays(1, &m_vertex_array);
        glBindVertexArray(m_vertex_array);

        float vertices[] = {
            -0.5f, -0.5f, 0.0f,
             0.5f, -0.5f, 0.0f,
             0.0f,  0.5f, 0.0f,
        };
        m_vertex_buffer.reset(VertexBuffer::create(vertices, sizeof(vertices)));
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);

        uint32_t indices[] = {
            0, 1, 2,
        };
        m_index_buffer.reset(IndexBuffer::create(indices, 3));

        std::string vertexSrc = R"(
            #version 330 core
            layout(location = 0) in vec3 a_Position;
            void main()
            {
                gl_Position = vec4(a_Position, 1.0);
            }
        )";

        std::string fragmentSrc = R"(
            #version 330 core
            layout(location = 0) out vec4 color;
            void main()
            {
                color = vec4(0.8, 0.2, 0.3, 1.0);
            }
        )";

        m_shader.reset(new Shader(vertexSrc, fragmentSrc));
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
            glClearColor(0.1f, 0.1f, 0.1f, 1);
            glClear(GL_COLOR_BUFFER_BIT);
            m_shader->bind();
            glBindVertexArray(m_vertex_array);
            glDrawElements(GL_TRIANGLES, m_index_buffer->get_count(), GL_UNSIGNED_INT, 0);

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