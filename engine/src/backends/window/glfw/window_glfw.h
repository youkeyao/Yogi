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

        void on_update() override;

        inline uint32_t get_width() const override { return m_data.width; }
        inline uint32_t get_height() const override { return m_data.height; }

        // Window attributes
        inline void set_event_callback(const EventCallbackFn& callback) override {
            m_data.event_callback = callback;
        };
        void set_vsync(bool enabled) override;
        bool is_vsync() const override;

        void* get_native_window() const override { return m_window; }

    private:
        GLFWwindow* m_window;
        Scope<GraphicsContext> m_context;

        struct WindowData {
            std::string title;
            uint32_t width, height;
            bool vsync;
            EventCallbackFn event_callback;
        };

        WindowData m_data;
    };

}