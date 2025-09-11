#pragma once

#include "Events/Event.h"

namespace Yogi
{

struct WindowProps
{
    std::string Title;
    uint32_t    Width;
    uint32_t    Height;
};

class YG_API Window
{
public:
    using EventCallbackFn = std::function<void(Event&)>;

    virtual ~Window() = default;

    virtual void Init()       = 0;
    virtual void OnUpdate()   = 0;
    virtual void WaitEvents() = 0;

    virtual void SetEventCallback(const EventCallbackFn& callback) = 0;

    virtual uint32_t GetWidth() const        = 0;
    virtual uint32_t GetHeight() const       = 0;
    virtual void*    GetNativeWindow() const = 0;

    static Handle<Window> Create(const WindowProps& props);
};

} // namespace Yogi