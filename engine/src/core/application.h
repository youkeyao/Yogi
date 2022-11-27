#pragma once

#include "core/window.h"
#include "core/layerstack.h"
#include "events/application_event.h"

namespace hazel {

    class Application
    {
    public:
        Application();
        virtual ~Application();
        void run();
        void on_event(Event& e);
        void push_layer(Layer* layer);
        void push_overlay(Layer* layer);
    private:
        bool on_window_close(WindowCloseEvent& e);
        bool on_window_resize(WindowResizeEvent& e);
        std::unique_ptr<Window> m_window;
        bool m_running = true;
        LayerStack m_layerstack;
    };

    extern Application* create_application();

}