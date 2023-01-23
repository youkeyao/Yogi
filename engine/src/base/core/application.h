#pragma once

#include "base/core/window.h"
#include "base/core/layerstack.h"
#include "base/core/timestep.h"
#include "base/events/application_event.h"
#include "base/imgui/imgui_layer.h"

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

        inline Window& get_window() { return*m_window; }
        inline static Application& get() { return *ms_instance; }
    private:
        bool on_window_close(WindowCloseEvent& e);
        bool on_window_resize(WindowResizeEvent& e);

        Scope<Window> m_window;
        ImGuiLayer* m_imgui_layer;
        bool m_running = true;
        bool m_minimized = false;
        LayerStack m_layerstack;
        float m_last_frame_time = 0.0f;

        static Application* ms_instance;
    };

    extern Application* create_application();

}