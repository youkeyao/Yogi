#include "WindowGLFW.h"

namespace Yogi
{

static bool s_isGLFWInitialized = false;

static void GLFWErrorCallback(int error, const char* description)
{
    YG_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
}

Handle<Window> Window::Create(const WindowProps& props) { return Handle<WindowGLFW>::Create(props); }

WindowGLFW::WindowGLFW(const WindowProps& props)
{
    m_data.Title  = props.Title;
    m_data.Width  = props.Width;
    m_data.Height = props.Height;
}

void WindowGLFW::Init()
{
    YG_CORE_INFO("Creating GLFW Window {0} ({1} {2})", m_data.Title, m_data.Width, m_data.Height);

    if (!s_isGLFWInitialized)
    {
        int success = glfwInit();
        YG_CORE_ASSERT(success, "Could not initialize GLFW!");
#ifndef YG_RENDERER_OPENGL
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif
        glfwSetErrorCallback(GLFWErrorCallback);
        s_isGLFWInitialized = true;
    }

    m_window = glfwCreateWindow(m_data.Width, m_data.Height, m_data.Title.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(m_window, &m_data);

    // Set GLFW callbacks
    glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int width, int height) {
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
        data.Width       = width;
        data.Height      = height;

        WindowResizeEvent event(width, height, nullptr);
        data.EventCallback(event);
    });

    glfwSetWindowCloseCallback(m_window, [](GLFWwindow* window) {
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

        WindowCloseEvent event(nullptr);
        data.EventCallback(event);
    });

    glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

        switch (action)
        {
            case GLFW_PRESS:
            {
                KeyPressedEvent event(key, 0, nullptr);
                data.EventCallback(event);
                break;
            }
            case GLFW_RELEASE:
            {
                KeyReleasedEvent event(key, nullptr);
                data.EventCallback(event);
                break;
            }
            case GLFW_REPEAT:
            {
                KeyPressedEvent event(key, 1, nullptr);
                data.EventCallback(event);
                break;
            }
        }
    });

    glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, int mods) {
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

        switch (action)
        {
            case GLFW_PRESS:
            {
                MouseButtonPressedEvent event(button, nullptr);
                data.EventCallback(event);
                break;
            }
            case GLFW_RELEASE:
            {
                MouseButtonReleasedEvent event(button, nullptr);
                data.EventCallback(event);
                break;
            }
        }
    });

    glfwSetScrollCallback(m_window, [](GLFWwindow* window, double x_offset, double y_offset) {
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

        MouseScrolledEvent event(x_offset, y_offset, nullptr);
        data.EventCallback(event);
    });

    glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double x_position, double y_position) {
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

        MouseMovedEvent event(x_position, y_position, nullptr);
        data.EventCallback(event);
    });
}

WindowGLFW::~WindowGLFW()
{
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void WindowGLFW::OnUpdate() { glfwPollEvents(); }

void WindowGLFW::WaitEvents() { glfwPollEvents(); }

} // namespace Yogi