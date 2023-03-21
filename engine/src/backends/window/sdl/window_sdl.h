#pragma once

#include <SDL.h>
#include "runtime/core/window.h"
#include "runtime/events/application_event.h"
#include "runtime/events/key_event.h"
#include "runtime/events/mouse_event.h"
#include "runtime/renderer/graphics_context.h"

namespace Yogi {

    class WindowSDL : public Window
    {
    public:
        WindowSDL(const WindowProps& props);
        virtual ~WindowSDL();

        void init() override;
        void on_update() override;

        void get_size(int32_t* width, int32_t* height) const override;
        void wait_events() override;

        // Window attributes
        inline void set_event_callback(const EventCallbackFn& callback) override {
            m_data.event_callback = callback;
        };

        void* get_native_window() const override { return m_window; }
        void* get_context() const override { return m_context.get(); }

        // OpenGL
        void make_gl_context() override;
        GLLoadProc gl_get_proc_address() const override;
        void gl_set_swap_interval(int32_t interval) override;
        void gl_swap_buffers() override;
        // Vulkan
        std::vector<const char*> vk_get_instance_extensions(uint32_t* count) const override;
        bool vk_create_surface(void* instance, void* surface) override;

    private:
        SDL_Window* m_window;
        SDL_Surface* screen_surface;
        Scope<GraphicsContext> m_context;

        struct WindowData {
            std::string title;
            uint32_t width, height;
            EventCallbackFn event_callback;
        };

        WindowData m_data;
    };

}