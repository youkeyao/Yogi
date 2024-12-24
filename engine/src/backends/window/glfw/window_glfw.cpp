#include "backends/window/glfw/window_glfw.h"

namespace Yogi {

static bool s_glfw_initialized = false;

static void glfw_error_callback(int error, const char *description)
{
    YG_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
}

Scope<Window> Window::create(const WindowProps &props)
{
    return CreateScope<WindowGLFW>(props);
}

WindowGLFW::WindowGLFW(const WindowProps &props)
{
    m_data.title = props.title;
    m_data.width = props.width;
    m_data.height = props.height;
}

void WindowGLFW::init()
{
    YG_CORE_INFO("Creating GLFW Window {0} ({1} {2})", m_data.title, m_data.width, m_data.height);

    if (!s_glfw_initialized) {
        int success = glfwInit();
        YG_CORE_ASSERT(success, "Could not initialize GLFW!");
#if YG_RENDERER_VULKAN
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif
        glfwWindowHint(GLFW_SAMPLES, 4);
        glfwSetErrorCallback(glfw_error_callback);
        s_glfw_initialized = true;
    }

    m_window = glfwCreateWindow(m_data.width, m_data.height, m_data.title.c_str(), nullptr, nullptr);

    m_context = GraphicsContext::create(this);
    m_context->init();
    glfwSetWindowUserPointer(m_window, &m_data);

    // Set GLFW callbacks
    glfwSetWindowSizeCallback(m_window, [](GLFWwindow *window, int width, int height) {
        WindowData &data = *(WindowData *)glfwGetWindowUserPointer(window);
        data.width = width;
        data.height = height;

        WindowResizeEvent event(width, height, nullptr);
        data.event_callback(event);
    });

    glfwSetWindowCloseCallback(m_window, [](GLFWwindow *window) {
        WindowData &data = *(WindowData *)glfwGetWindowUserPointer(window);

        WindowCloseEvent event(nullptr);
        data.event_callback(event);
    });

    glfwSetKeyCallback(m_window, [](GLFWwindow *window, int key, int scancode, int action, int mods) {
        WindowData &data = *(WindowData *)glfwGetWindowUserPointer(window);

        switch (action) {
        case GLFW_PRESS: {
            KeyPressedEvent event(key, 0, nullptr);
            data.event_callback(event);
            break;
        }
        case GLFW_RELEASE: {
            KeyReleasedEvent event(key, nullptr);
            data.event_callback(event);
            break;
        }
        case GLFW_REPEAT: {
            KeyPressedEvent event(key, 1, nullptr);
            data.event_callback(event);
            break;
        }
        }
    });

    glfwSetMouseButtonCallback(m_window, [](GLFWwindow *window, int button, int action, int mods) {
        WindowData &data = *(WindowData *)glfwGetWindowUserPointer(window);

        switch (action) {
        case GLFW_PRESS: {
            MouseButtonPressedEvent event(button, nullptr);
            data.event_callback(event);
            break;
        }
        case GLFW_RELEASE: {
            MouseButtonReleasedEvent event(button, nullptr);
            data.event_callback(event);
            break;
        }
        }
    });

    glfwSetScrollCallback(m_window, [](GLFWwindow *window, double x_offset, double y_offset) {
        WindowData &data = *(WindowData *)glfwGetWindowUserPointer(window);

        MouseScrolledEvent event(x_offset, y_offset, nullptr);
        data.event_callback(event);
    });

    glfwSetCursorPosCallback(m_window, [](GLFWwindow *window, double x_position, double y_position) {
        WindowData &data = *(WindowData *)glfwGetWindowUserPointer(window);

        MouseMovedEvent event(x_position, y_position, nullptr);
        data.event_callback(event);
    });
}

WindowGLFW::~WindowGLFW()
{
    m_context->shutdown();
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void WindowGLFW::on_update()
{
    glfwPollEvents();
    m_context->swap_buffers();
}

void WindowGLFW::get_size(int32_t *width, int32_t *height) const
{
    glfwGetFramebufferSize(m_window, width, height);
}

void WindowGLFW::wait_events()
{
    glfwPollEvents();
}

// OpenGL
void WindowGLFW::make_gl_context()
{
    glfwMakeContextCurrent(m_window);
}
WindowGLFW::GLLoadProc WindowGLFW::gl_get_proc_address() const
{
    return (GLLoadProc)glfwGetProcAddress;
}
void WindowGLFW::gl_set_swap_interval(int32_t interval)
{
    glfwSwapInterval(interval);
}
void WindowGLFW::gl_swap_buffers()
{
    glfwSwapBuffers(m_window);
}
// Vulkan
std::vector<const char *> WindowGLFW::vk_get_instance_extensions(uint32_t *count) const
{
    const char **extensions = glfwGetRequiredInstanceExtensions(count);
    return std::vector<const char *>{ extensions, extensions + *count };
}
bool WindowGLFW::vk_create_surface(void *instance, void *surface)
{
    bool status = false;
#if YG_RENDERER_VULKAN
    status = glfwCreateWindowSurface((VkInstance)instance, m_window, nullptr, (VkSurfaceKHR *)surface) == VK_SUCCESS;
#endif
    return status;
}

}  // namespace Yogi