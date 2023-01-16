#include "core/application.h"
#include "renderer/renderer.h"
#include <GLFW/glfw3.h>

namespace hazel {

    Application* Application::ms_instance = nullptr;

    Application::Application()
    {
        HZ_CORE_ASSERT(!ms_instance, "Application already exists!");
        ms_instance = this;

        m_window = Scope<Window>(Window::create());
        m_window->set_event_callback(HZ_BIND_EVENT_FN(Application::on_event));

        Renderer::init();

        m_imgui_layer = new ImGuiLayer();
        push_overlay(m_imgui_layer);
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
        dispatcher.dispatch<WindowCloseEvent>(HZ_BIND_EVENT_FN(Application::on_window_close));
        dispatcher.dispatch<WindowResizeEvent>(HZ_BIND_EVENT_FN(Application::on_window_resize));

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
            float time = (float)glfwGetTime();
            TimeStep timestep = time - m_last_frame_time;
            m_last_frame_time = time;

            if (!m_minimized) {
                for (Layer* layer : m_layerstack) {
                    layer->on_update(timestep);
                }
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

    bool Application::on_window_resize(WindowResizeEvent& e)
    {
        if (e.get_width() == 0 || e.get_height() == 0) {
            m_minimized = true;
            return false;
        }

        m_minimized = false;
        Renderer::on_window_resize(e.get_width(), e.get_height());
        return false;
    }

}