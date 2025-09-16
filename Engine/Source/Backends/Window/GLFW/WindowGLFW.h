#pragma once

#include "Core/Window.h"
#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"
#include "Renderer/RHI/IDeviceContext.h"

#include <GLFW/glfw3.h>

namespace Yogi
{

class WindowGLFW : public Window
{
public:
    WindowGLFW(const WindowProps& props);
    virtual ~WindowGLFW();

    void OnUpdate() override;

    inline void SetEventCallback(const EventCallbackFn& callback) override { m_data.EventCallback = callback; };

    uint32_t GetWidth() const override { return m_data.Width; }
    uint32_t GetHeight() const override { return m_data.Height; }
    void*    GetNativeWindow() const override { return m_window; }

private:
    GLFWwindow* m_window;

    struct WindowData
    {
        std::string     Title;
        uint32_t        Width, Height;
        EventCallbackFn EventCallback;
    };

    WindowData m_data;
};

} // namespace Yogi