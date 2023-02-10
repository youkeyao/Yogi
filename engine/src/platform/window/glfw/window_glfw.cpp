#include "platform/window/glfw/window_glfw.h"
#if YG_RENDERER_API == 1
    #include "platform/renderer/opengl/opengl_context.h"
#endif

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

        init(props);
    }

    WindowGLFW::~WindowGLFW()
    {
        YG_PROFILE_FUNCTION();

        shutdown();
    }

    void WindowGLFW::init(const WindowProps& props)
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
            glfwSetErrorCallback(glfw_error_callback);
            s_glfw_initialized = true;
        }

        {
            YG_PROFILE_SCOPE("glfwCreateWindow");
            
            m_window = glfwCreateWindow(m_data.width, m_data.height, m_data.title.c_str(), nullptr, nullptr);
        }

        #if YG_RENDERER_API == 1
            m_context = new OpenGLContext(m_window);
        #endif
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
        YG_PROFILE_FUNCTION();

        glfwDestroyWindow(m_window);
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