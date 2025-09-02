#include "Core/Application.h"

namespace Yogi
{

Application* Application::s_instance = nullptr;

Application::Application(const std::string& name)
{
    Yogi::Log::Init();
    YG_PROFILE_FUNCTION();

    YG_CORE_ASSERT(!s_instance, "Application already exists!");
    s_instance = this;

    m_window = Window::Create(WindowProps{ name, 1280, 720 });
    m_window->Init();
    m_window->SetEventCallback(YG_BIND_EVENT_FN(Application::OnEvent));

    m_context   = IDeviceContext::Create();
    m_swapChain = ISwapChain::Create(SwapChainDesc{ m_window->GetWidth(),
                                                    m_window->GetHeight(),
                                                    ITexture::Format::R8G8B8A8_SRGB,
                                                    ITexture::Format::D32_FLOAT,
                                                    SampleCountFlagBits::Count4,
                                                    CreateView(m_window) });
}

Application::~Application()
{
    YG_PROFILE_FUNCTION();

    for (auto& layer : m_layers)
    {
        layer->OnDetach();
        layer = nullptr;
    }
    m_layers.clear();

    m_swapChain = nullptr;
    m_context   = nullptr;
}

void Application::PushLayer(Scope<Layer> layer)
{
    YG_PROFILE_FUNCTION();

    m_layers.push_back(std::move(layer));
    m_layers.back()->OnAttach();
}

void Application::OnEvent(Event& e)
{
    YG_PROFILE_FUNCTION();

    EventDispatcher dispatcher(e);

    for (auto it = m_layers.end(); it != m_layers.begin();)
    {
        (*--it)->OnEvent(e);
        if (e.m_handled)
        {
            break;
        }
    }

    dispatcher.dispatch<WindowCloseEvent>(YG_BIND_EVENT_FN(Application::OnWindowClose));
    dispatcher.dispatch<WindowResizeEvent>(YG_BIND_EVENT_FN(Application::OnWindowResize));
}

void Application::Close() { m_isRunning = false; }

void Application::Run()
{
    YG_PROFILE_FUNCTION();

    m_lastFrameTime = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::steady_clock::now())
                          .time_since_epoch()
                          .count() *
        0.000001f;
    while (m_isRunning)
    {
        YG_PROFILE_SCOPE("RunLoop");

        m_swapChain->AcquireNextImage();
        float time = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::steady_clock::now())
                         .time_since_epoch()
                         .count() *
            0.000001f;
        Timestep timestep = time - m_lastFrameTime;
        m_lastFrameTime   = time;

        if (!m_isMinimized)
        {
            {
                YG_PROFILE_SCOPE("LayerStack on_update");

                for (auto& layer : m_layers)
                {
                    layer->OnUpdate(timestep);
                }
            }
        }

        m_swapChain->Present();
        m_window->OnUpdate();
    }
}

bool Application::OnWindowClose(WindowCloseEvent& e)
{
    Close();
    return true;
}

bool Application::OnWindowResize(WindowResizeEvent& e)
{
    YG_PROFILE_FUNCTION();

    if (e.GetWidth() == 0 || e.GetHeight() == 0)
    {
        m_isMinimized = true;
        return false;
    }

    m_isMinimized = false;
    return false;
}

} // namespace Yogi