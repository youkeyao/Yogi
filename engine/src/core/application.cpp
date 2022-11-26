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

    void Application::on_event(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::on_window_close));
        HZ_CORE_INFO("{0}", e);
    }

    void Application::run()
    {
        while (m_running) {
            m_window->on_update();
        }
    }

    bool Application::on_window_close(WindowCloseEvent& e)
    {
        m_running = false;
        return true;
    }

}