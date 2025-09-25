#include "Core/Application.h"

#include "Resources/AssetManager/AssetManager.h"
#include "Resources/ResourceManager/ResourceManager.h"

namespace Yogi
{

Application* Application::s_instance = nullptr;

Application::Application(const std::string& name)
{
    Log::Init();
    YG_PROFILE_FUNCTION();

    YG_CORE_ASSERT(!s_instance, "Application already exists!");
    s_instance = this;

    m_window = Window::Create(WindowProps{ name, 1280, 720 });
    m_window->SetEventCallback(YG_BIND_FN(Application::OnEvent));

    m_context   = Handle<IDeviceContext>::Create(Ref<Window>::Create(m_window));
    m_swapChain = Handle<ISwapChain>::Create(SwapChainDesc{ m_window->GetWidth(),
                                                            m_window->GetHeight(),
                                                            ITexture::Format::B8G8R8A8_UNORM,
                                                            ITexture::Format::D32_FLOAT,
                                                            SampleCountFlagBits::Count4,
                                                            Ref<Window>::Create(m_window) });
}

Application::~Application()
{
    YG_PROFILE_FUNCTION();

    for (auto& layer : m_layers)
    {
        layer = nullptr;
    }
    m_layers.clear();

    AssetManager::Clear();
    ResourceManager::Clear();

    m_swapChain = nullptr;
    m_context   = nullptr;
}

void Application::PushLayer(Handle<Layer>&& layer)
{
    YG_PROFILE_FUNCTION();

    m_layers.push_back(std::move(layer));
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

    dispatcher.Dispatch<WindowCloseEvent>(YG_BIND_FN(Application::OnWindowClose));
    dispatcher.Dispatch<WindowResizeEvent>(YG_BIND_FN(Application::OnWindowResize));
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

        float time = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::steady_clock::now())
                         .time_since_epoch()
                         .count() *
            0.000001f;
        Timestep timestep = time - m_lastFrameTime;
        m_lastFrameTime   = time;

        if (!m_isMinimized)
        {
            m_swapChain->AcquireNextImage();

            {
                YG_PROFILE_SCOPE("LayerStack on_update");

                for (auto& layer : m_layers)
                {
                    layer->OnUpdate(timestep);
                }
            }

            m_swapChain->Present();
        }

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
    m_swapChain->Resize(e.GetWidth(), e.GetHeight());
    return false;
}

} // namespace Yogi