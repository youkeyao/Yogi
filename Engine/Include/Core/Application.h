#pragma once

#include "Core/Timestep.h"
#include "Core/Layer.h"
#include "Core/Window.h"
#include "Events/ApplicationEvent.h"
#include "Renderer/RHI/IDeviceContext.h"
#include "Renderer/RHI/ISwapChain.h"

namespace Yogi
{

class YG_API Application
{
public:
    Application(const std::string& name = "Yogi Engine");
    virtual ~Application();

    void Run();
    void Close();
    void OnEvent(Event& e);
    void PushLayer(Scope<Layer> layer);

    inline View<Window>         GetWindow() { return CreateView(m_window); }
    inline View<IDeviceContext> GetContext() { return CreateView(m_context); }
    inline View<ISwapChain>     GetSwapChain() { return CreateView(m_swapChain); }

    inline static Application& GetInstance() { return *s_instance; }

private:
    bool OnWindowClose(WindowCloseEvent& e);
    bool OnWindowResize(WindowResizeEvent& e);

private:
    Scope<Window>             m_window;
    Scope<IDeviceContext>     m_context;
    Scope<ISwapChain>         m_swapChain;
    std::vector<Scope<Layer>> m_layers;

    float m_lastFrameTime = 0.0f;
    bool  m_isRunning     = true;
    bool  m_isMinimized   = false;

    static Application* s_instance;
};

extern Application* CreateApplication();

} // namespace Yogi