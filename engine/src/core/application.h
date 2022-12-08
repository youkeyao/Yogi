#pragma once

#include "core/window.h"
#include "core/layerstack.h"
#include "events/application_event.h"
#include "imgui/imgui_layer.h"

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

        std::unique_ptr<Window> m_window;
        ImGuiLayer* m_imgui_layer;
        bool m_running = true;
        LayerStack m_layerstack;

        static Application* ms_instance;
    };

    extern Application* create_application();

}