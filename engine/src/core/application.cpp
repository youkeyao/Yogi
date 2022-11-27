#include "core/application.h"

namespace hazel {

    #define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)

    Application::Application()
    {
        m_window = std::unique_ptr<Window>(Window::create());
        m_window->set_event_callback(BIND_EVENT_FN(Application::on_event));
    }

    Application::~Application()
    {

    }

    void Application::push_layer(Layer* layer)
    {
        m_layerstack.push_layer(layer);
    }

    void Application::push_overlay(Layer* layer)
    {
        m_layerstack.push_overlay(layer);
    }

    void Application::on_event(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::on_window_close));
        HZ_CORE_INFO("{0}", e);

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
            for (Layer* layer : m_layerstack) {
                layer->on_update();
            }
            m_window->on_update();
        }
    }

    bool Application::on_window_close(WindowCloseEvent& e)
    {
        m_running = false;
        return true;
    }

}