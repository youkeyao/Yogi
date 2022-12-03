#include "platform/window/window_glfw.h"
#include "platform/opengl/opengl_context.h"

namespace hazel {

    static bool s_glfw_initialized = false;

    static void glfw_error_callback(int error, const char* description) {
        HZ_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
    }

    Window* Window::create(const WindowProps& props)
    {
        return new WindowGLFW(props);
    }

    WindowGLFW::WindowGLFW(const WindowProps& props)
    {
        init(props);
    }

    WindowGLFW::~WindowGLFW()
    {
        shutdown();
    }

    void WindowGLFW::init(const WindowProps& props)
    {
        m_data.title = props.title;
        m_data.width = props.width;
        m_data.height = props.height;

        HZ_CORE_INFO("Creating Window {0} ({1} {2})", props.title, props.width, props.height);

        if (!s_glfw_initialized) {
            int success = glfwInit();
            HZ_CORE_ASSERT(success, "Could not initialize GLFW!");
            glfwSetErrorCallback(glfw_error_callback);
            s_glfw_initialized = true;
        }

        m_window = glfwCreateWindow(m_data.width, m_data.height, m_data.title.c_str(), nullptr, nullptr);
        m_context = new OpenGLContext(m_window);
        m_context->init();

        glfwSetWindowUserPointer(m_window, &m_data);
        set_vsync(true);

        // Set GLFW callbacks
        glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int width, int height) {
            WindowData& data = *(WindowData*) glfwGetWindowUserPointer(window);
            data.width = width;
            data.height = height;

            WindowResizeEvent event(width, height);
            data.event_callback(event);
        });

        glfwSetWindowCloseCallback(m_window, [](GLFWwindow* window) {
            WindowData& data = *(WindowData*) glfwGetWindowUserPointer(window);

            WindowCloseEvent event;
            data.event_callback(event);
        });

        glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
            WindowData& data = *(WindowData*) glfwGetWindowUserPointer(window);

            switch (action) {
                case GLFW_PRESS: {
                    KeyPressedEvent event(key, 0);
                    data.event_callback(event);
                    break;
                }
                case GLFW_RELEASE: {
                    KeyReleasedEvent event(key);
                    data.event_callback(event);
                    break;
                }
                case GLFW_REPEAT: {
                    KeyPressedEvent event(key, 1);
                    data.event_callback(event);
                    break;
                }
            }
        });

        glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, int mods) {
            WindowData& data = *(WindowData*) glfwGetWindowUserPointer(window);

            switch (action) {
                case GLFW_PRESS: {
                    MouseButtonPressedEvent event(button);
                    data.event_callback(event);
                    break;
                }
                case GLFW_RELEASE: {
                    MouseButtonReleasedEvent event(button);
                    data.event_callback(event);
                    break;
                }
            }
        });

        glfwSetScrollCallback(m_window, [](GLFWwindow* window, double x_offset, double y_offset) {
            WindowData& data = *(WindowData*) glfwGetWindowUserPointer(window);

            MouseScrolledEvent event(x_offset, y_offset);
            data.event_callback(event);
        });

        glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double x_position, double y_position) {
            WindowData& data = *(WindowData*) glfwGetWindowUserPointer(window);

            MouseMovedEvent event(x_position, y_position);
            data.event_callback(event);
        });
    }

    void WindowGLFW::shutdown()
    {
        glfwDestroyWindow(m_window);
    }

    void WindowGLFW::on_update()
    {
        glfwPollEvents();
        m_context->swap_buffers();
    }

    void WindowGLFW::set_vsync(bool enabled)
    {
        if (enabled) {
            glfwSwapInterval(1);
        }
        else {
            glfwSwapInterval(0);
        }

        m_data.vsync = enabled;
    }

    bool WindowGLFW::is_vsync() const { return m_data.vsync; }

}