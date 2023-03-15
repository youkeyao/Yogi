#include "runtime/core/application.h"
// #include "runtime/renderer/renderer.h"
#include "runtime/renderer/render_command.h"
#include "runtime/renderer/shader.h"
#include "runtime/renderer/buffer.h"
#include "runtime/renderer/texture.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Yogi {

    Application* Application::ms_instance = nullptr;

    Application::Application(const std::string& name)
    {
        YG_PROFILE_FUNCTION();

        YG_CORE_ASSERT(!ms_instance, "Application already exists!");
        ms_instance = this;

        m_window = Window::create(WindowProps(name));
        m_window->set_event_callback(YG_BIND_EVENT_FN(Application::on_event));

        // Renderer::init();
    }

    Application::~Application()
    {
        YG_PROFILE_FUNCTION();

        // Renderer::shutdown();
    }

    void Application::push_layer(Layer* layer)
    {
        YG_PROFILE_FUNCTION();

        m_layerstack.push_layer(layer);
        layer->on_attach();
    }

    void Application::push_overlay(Layer* layer)
    {
        YG_PROFILE_FUNCTION();

        m_layerstack.push_overlay(layer);
        layer->on_attach();
    }

    void Application::on_event(Event& e)
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

        m_last_frame_time = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()).time_since_epoch().count() * 0.000001f;
        Ref<Shader> s = Shader::create("editor", {"vert", "frag"});
        s->bind();
        float vertices[] = {
            -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
            0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
            -0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f
        };
        uint32_t indices[] = {
            0, 1, 2, 2, 3, 0
        };
        Ref<VertexBuffer> vertex_buffer = VertexBuffer::create(vertices, sizeof(vertices));
        vertex_buffer->bind();
        Ref<IndexBuffer> index_buffer = IndexBuffer::create(indices, sizeof(indices) / sizeof(uint32_t));
        index_buffer->bind();
        struct UniformBufferObject {
            glm::mat4 model;
            glm::mat4 view;
            glm::mat4 proj;
        } ubo;
        Ref<UniformBuffer> uniform_buffer = UniformBuffer::create(sizeof(UniformBufferObject));
        uniform_buffer->bind(0);
        // Ref<Texture2D> texture = Texture2D::create("../sandbox/assets/textures/checkerboard.png");
        while (m_running) {
            YG_PROFILE_SCOPE("RunLoop");

            float time = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()).time_since_epoch().count() * 0.000001f;
            Timestep timestep = time - m_last_frame_time;
            m_last_frame_time = time;

            if (!m_minimized) {
                {
                    YG_PROFILE_SCOPE("LayerStack on_update");

                    for (Layer* layer : m_layerstack) {
                        layer->on_update(timestep);
                    }
                }
            }
            RenderCommand::set_clear_color({0.8, 0.2, 0.3, 1.0});
            ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            ubo.view = glm::lookAt(glm::vec3(5.0f, 5.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            ubo.proj = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 10.0f);
            ubo.proj[1][1] *= -1;
            uniform_buffer->set_data(&ubo, sizeof(ubo));
            RenderCommand::draw_indexed(index_buffer);

            m_window->on_update();
        }
    }

    bool Application::on_window_close(WindowCloseEvent& e)
    {
        close();
        return true;
    }

    bool Application::on_window_resize(WindowResizeEvent& e)
    {
        YG_PROFILE_FUNCTION();
        
        if (e.get_width() == 0 || e.get_height() == 0) {
            m_minimized = true;
            return false;
        }

        m_minimized = false;
        // Renderer::on_window_resize(e.get_width(), e.get_height());
        RenderCommand::set_viewport(0, 0, e.get_width(), e.get_height());
        return false;
    }

}