#pragma once

#include "core/window.h"
#include "events/application_event.h"
#include "events/key_event.h"
#include "events/mouse_event.h"

namespace hazel {

    class Application
    {
    public:
        Application();
        virtual ~Application();
        void run();
        void on_event(Event& e);
    private:
        bool on_window_close(WindowCloseEvent& e);
        bool on_window_resize(WindowResizeEvent& e);
        std::unique_ptr<Window> m_window;
        bool m_running = true;
    };

    extern Application* create_application();

}