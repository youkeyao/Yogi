#pragma once

#include "runtime/core/window.h"
#include "runtime/core/layerstack.h"
#include "runtime/core/timestep.h"
#include "runtime/events/application_event.h"

namespace Yogi {

    class Application
    {
    public:
        Application(const std::string& name = "Yogi Engine");
        virtual ~Application();
        void run();
        void close();
        void on_event(Event& e);
        void push_layer(Layer* layer);
        void push_overlay(Layer* layer);

        inline Window& get_window() { return *m_window; }
        inline static Application& get() { return *ms_instance; }
    private:
        bool on_window_close(WindowCloseEvent& e);
        bool on_window_resize(WindowResizeEvent& e);

        Scope<Window> m_window;
        bool m_running = true;
        bool m_minimized = false;
        LayerStack m_layerstack;
        float m_last_frame_time = 0.0f;

        static Application* ms_instance;
    };

    extern Application* create_application();

}