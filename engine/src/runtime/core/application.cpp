#include "runtime/core/application.h"

#include "runtime/renderer/renderer.h"
#include "runtime/resources/asset_manager.h"

#include <glm/glm.hpp>

namespace Yogi {

Application *Application::ms_instance = nullptr;

Application::Application(const std::string &name)
{
    YG_PROFILE_FUNCTION();

    YG_CORE_ASSERT(!ms_instance, "Application already exists!");
    ms_instance = this;

    m_window = Window::create(WindowProps(name));
    m_window->init();
    m_window->set_event_callback(YG_BIND_EVENT_FN(Application::on_event));

    PipelineManager::init();
    AssetManager::init(YG_ASSET_DIR);
    Renderer::init();
}

Application::~Application()
{
    YG_PROFILE_FUNCTION();

    for (Layer *layer : m_layerstack) {
        layer->on_detach();
    }
    AssetManager::clear();
    PipelineManager::clear();
    Renderer::shutdown();
}

void Application::push_layer(Layer *layer)
{
    YG_PROFILE_FUNCTION();

    m_layerstack.push_layer(layer);
    layer->on_attach();
}

void Application::push_overlay(Layer *layer)
{
    YG_PROFILE_FUNCTION();

    m_layerstack.push_overlay(layer);
    layer->on_attach();
}

void Application::on_event(Event &e)
{
    YG_PROFILE_FUNCTION();

    EventDispatcher dispatcher(e);

    for (auto it = m_layerstack.end(); it != m_layerstack.begin();) {
        (*--it)->on_event(e);
        if (e.m_handled) {
            break;
        }
    }

    dispatcher.dispatch<WindowCloseEvent>(YG_BIND_EVENT_FN(Application::on_window_close));
    dispatcher.dispatch<WindowResizeEvent>(YG_BIND_EVENT_FN(Application::on_window_resize));
}

void Application::close()
{
    m_running = false;
}

void Application::run()
{
    YG_PROFILE_FUNCTION();

    m_last_frame_time =
        std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()).time_since_epoch().count() *
        0.000001f;
    while (m_running) {
        YG_PROFILE_SCOPE("RunLoop");

        float time = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::steady_clock::now())
                         .time_since_epoch()
                         .count() *
                     0.000001f;
        Timestep timestep = time - m_last_frame_time;
        m_last_frame_time = time;

        if (!m_minimized) {
            {
                YG_PROFILE_SCOPE("LayerStack on_update");

                for (Layer *layer : m_layerstack) {
                    layer->on_update(timestep);
                }
            }
        }

        m_window->on_update();
    }
}

bool Application::on_window_close(WindowCloseEvent &e)
{
    close();
    return true;
}

bool Application::on_window_resize(WindowResizeEvent &e)
{
    YG_PROFILE_FUNCTION();

    if (e.get_width() == 0 || e.get_height() == 0) {
        m_minimized = true;
        return false;
    }

    m_minimized = false;
    return false;
}

}  // namespace Yogi