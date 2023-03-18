#pragma once

#include "runtime/events/event.h"

namespace Yogi {

    struct WindowProps
    {
        std::string title;
        uint32_t width;
        uint32_t height;

        WindowProps(const std::string& title = "Yogi Engine",
                    uint32_t width = 1280,
                    uint32_t height = 720)
            : title(title), width(width), height(height) {}
    };
    
    class Window
    {
    public:
        using EventCallbackFn = std::function<void(Event&)>;

        virtual ~Window() = default;

        virtual void init() = 0;
        virtual void on_update() = 0;

        virtual uint32_t get_width() const = 0;
        virtual uint32_t get_height() const = 0;

        // Window attributes
        virtual void set_event_callback(const EventCallbackFn& callback) = 0;

        virtual void* get_native_window() const = 0;
        virtual void* get_context() const = 0;

        static Scope<Window> create(const WindowProps& props = WindowProps());
    };

}