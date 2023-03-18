#pragma once

#include <GLFW/glfw3.h>
#include "runtime/core/window.h"
#include "runtime/events/application_event.h"
#include "runtime/events/key_event.h"
#include "runtime/events/mouse_event.h"
#include "runtime/renderer/graphics_context.h"

namespace Yogi {

    class WindowGLFW : public Window
    {
    public:
        WindowGLFW(const WindowProps& props);
        virtual ~WindowGLFW();

        void init() override;
        void on_update() override;

        inline uint32_t get_width() const override { return m_data.width; }
        inline uint32_t get_height() const override { return m_data.height; }

        // Window attributes
        inline void set_event_callback(const EventCallbackFn& callback) override {
            m_data.event_callback = callback;
        };

        void* get_native_window() const override { return m_window; }
        void* get_context() const override { return m_context.get(); }

    private:
        GLFWwindow* m_window;
        Scope<GraphicsContext> m_context;

        struct WindowData {
            std::string title;
            uint32_t width, height;
            EventCallbackFn event_callback;
        };

        WindowData m_data;
    };

}