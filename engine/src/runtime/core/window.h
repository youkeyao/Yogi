#pragma once

#include "runtime/events/event.h"

namespace Yogi {

struct WindowProps
{
    std::string title;
    uint32_t    width;
    uint32_t    height;

    WindowProps(const std::string &title = "Yogi Engine", uint32_t width = 1280, uint32_t height = 720)
        : title(title), width(width), height(height)
    {
    }
};

class Window
{
protected:
    typedef void *(*GLLoadProc)(const char *name);

public:
    using EventCallbackFn = std::function<void(Event &)>;

    virtual ~Window() = default;

    virtual void init() = 0;
    virtual void on_update() = 0;

    virtual void get_size(int32_t *width, int32_t *height) const = 0;
    virtual void wait_events() = 0;

    // Window attributes
    virtual void set_event_callback(const EventCallbackFn &callback) = 0;

    virtual void *get_native_window() const = 0;
    virtual void *get_context() const = 0;

    // OpenGL
    virtual void       make_gl_context() = 0;
    virtual GLLoadProc gl_get_proc_address() const = 0;
    virtual void       gl_set_swap_interval(int32_t interval) = 0;
    virtual void       gl_swap_buffers() = 0;
    // Vulkan
    virtual std::vector<const char *> vk_get_instance_extensions(uint32_t *count) const = 0;
    virtual bool                      vk_create_surface(void *instance, void *surface) = 0;

    static Scope<Window> create(const WindowProps &props = WindowProps());
};

}  // namespace Yogi