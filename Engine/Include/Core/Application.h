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
    void PushLayer(Owner<Layer>&& layer);

    WRef<Layer> AcquireLayer(const std::string& name);

    inline Window*         GetWindow() const { return m_window.Get(); }
    inline IDeviceContext* GetContext() const { return m_context.Get(); }
    inline ISwapChain*     GetSwapChain() const { return m_swapChain.Get(); }

    inline static Application& GetInstance() { return *s_instance; }

private:
    bool OnWindowClose(WindowCloseEvent& e);
    bool OnWindowResize(WindowResizeEvent& e);

private:
    Owner<Window>             m_window    = nullptr;
    Owner<IDeviceContext>     m_context   = nullptr;
    Owner<ISwapChain>         m_swapChain = nullptr;
    std::vector<Owner<Layer>> m_layers;

    float m_lastFrameTime = 0.0f;
    bool  m_isRunning     = true;
    bool  m_isMinimized   = false;

    static Application* s_instance;
};

extern Application* CreateApplication();

} // namespace Yogi