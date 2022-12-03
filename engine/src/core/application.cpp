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

        glGenBuffers(1, &m_vertex_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
        float vertices[] = {
            -0.5f, -0.5f, 0.0f,
             0.5f, -0.5f, 0.0f,
             0.0f,  0.5f, 0.0f,
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);

        glGenBuffers(1, &m_index_buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buffer);
        unsigned int indices[] = {
            0, 1, 2,
        };
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
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
            glBindVertexArray(m_vertex_array);
            glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);

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