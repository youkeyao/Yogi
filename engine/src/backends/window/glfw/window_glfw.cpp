#include "backends/window/glfw/window_glfw.h"

namespace Yogi {

    static bool s_glfw_initialized = false;

    static void glfw_error_callback(int error, const char* description) {
        YG_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
    }

    Scope<Window> Window::create(const WindowProps& props)
    {
        return CreateScope<WindowGLFW>(props);
    }

    WindowGLFW::WindowGLFW(const WindowProps& props)
    {
        YG_PROFILE_FUNCTION();

        m_data.title = props.title;
        m_data.width = props.width;
        m_data.height = props.height;

        YG_CORE_INFO("Creating Window {0} ({1} {2})", props.title, props.width, props.height);

        if (!s_glfw_initialized) {
            YG_PROFILE_SCOPE("glfwInit");

            int success = glfwInit();
            YG_CORE_ASSERT(success, "Could not initialize GLFW!");
            #if YG_RENDERER_API == YG_RENDERER_VULKAN
                glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            #endif
            glfwSetErrorCallback(glfw_error_callback);
            s_glfw_initialized = true;
        }

        {
            YG_PROFILE_SCOPE("glfwCreateWindow");
            
            m_window = glfwCreateWindow(m_data.width, m_data.height, m_data.title.c_str(), nullptr, nullptr);
        }

        m_context = GraphicsContext::create(this);
        glfwSetWindowUserPointer(m_window, &m_data);
        #if YG_RENDERER_API == YG_RENDERER_OPENGL
            set_vsync(true);
        #endif

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

    WindowGLFW::~WindowGLFW()
    {
        YG_PROFILE_FUNCTION();

        glfwDestroyWindow(m_window);
        glfwTerminate();
    }

    void WindowGLFW::on_update()
    {
        YG_PROFILE_FUNCTION();

        glfwPollEvents();
        m_context->swap_buffers();
    }

    void WindowGLFW::set_vsync(bool enabled)
    {
        YG_PROFILE_FUNCTION();
        
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