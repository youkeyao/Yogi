#include "core/application.h"
#include "renderer/renderer.h"
#include <GLFW/glfw3.h>

namespace hazel {

    Application* Application::ms_instance = nullptr;

    Application::Application()
    {
        HZ_PROFILE_FUNCTION();

        HZ_CORE_ASSERT(!ms_instance, "Application already exists!");
        ms_instance = this;

        m_window = Window::create();
        m_window->set_event_callback(HZ_BIND_EVENT_FN(Application::on_event));

        Renderer::init();

        m_imgui_layer = new ImGuiLayer();
        push_overlay(m_imgui_layer);
    }

    Application::~Application()
    {
        HZ_PROFILE_FUNCTION();

        Renderer::shutdown();
    }

    void Application::push_layer(Layer* layer)
    {
        HZ_PROFILE_FUNCTION();

        m_layerstack.push_layer(layer);
        layer->on_attach();
    }

    void Application::push_overlay(Layer* layer)
    {
        HZ_PROFILE_FUNCTION();

        m_layerstack.push_overlay(layer);
        layer->on_attach();
    }

    void Application::on_event(Event& e)
    {
        HZ_PROFILE_FUNCTION();

        EventDispatcher dispatcher(e);
        dispatcher.dispatch<WindowCloseEvent>(HZ_BIND_EVENT_FN(Application::on_window_close));
        dispatcher.dispatch<WindowResizeEvent>(HZ_BIND_EVENT_FN(Application::on_window_resize));

        for (auto it = m_layerstack.end(); it != m_layerstack.begin();) {
            (*--it)->on_event(e);
            if (e.m_handled) {
                break;
            }
        }
    }

    void Application::run()
    {
        HZ_PROFILE_FUNCTION();

        while (m_running) {
            HZ_PROFILE_SCOPE("RunLoop");

            float time = (float)glfwGetTime();
            TimeStep timestep = time - m_last_frame_time;
            m_last_frame_time = time;

            if (!m_minimized) {
                {
                    HZ_PROFILE_SCOPE("LayerStack on_update");

                    for (Layer* layer : m_layerstack) {
                        layer->on_update(timestep);
                    }
                }
                m_imgui_layer->begin();
                {
                    HZ_PROFILE_SCOPE("LayerStack on_imgui_render");

                    for (Layer* layer : m_layerstack) {
                        layer->on_imgui_render();
                    }
                }
                m_imgui_layer->end();
            }

            m_window->on_update();
        }
    }

    bool Application::on_window_close(WindowCloseEvent& e)
    {
        m_running = false;
        return true;
    }

    bool Application::on_window_resize(WindowResizeEvent& e)
    {
        HZ_PROFILE_FUNCTION();
        
        if (e.get_width() == 0 || e.get_height() == 0) {
            m_minimized = true;
            return false;
        }

        m_minimized = false;
        Renderer::on_window_resize(e.get_width(), e.get_height());
        return false;
    }

}