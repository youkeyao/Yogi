#include "platform/input/input.h"

namespace hazel {

    Input* Input::ms_instance = new InputGLFW();

    bool InputGLFW::is_key_pressed_impl(int keycode)
    {
        auto window = static_cast<GLFWwindow*>(Application::get().get_window().get_native_window());
        int state = glfwGetKey(window, keycode);
        return state == GLFW_PRESS || state == GLFW_REPEAT;
    }

    bool InputGLFW::is_mouse_button_pressed_impl(int button)
    {
        auto window = static_cast<GLFWwindow*>(Application::get().get_window().get_native_window());
        int state = glfwGetMouseButton(window, button);
        return state == GLFW_PRESS;
    }

    std::pair<float, float> InputGLFW::get_mouse_position_impl()
    {
        auto window = static_cast<GLFWwindow*>(Application::get().get_window().get_native_window());
        double x_pos, y_pos;
        glfwGetCursorPos(window, &x_pos, &y_pos);
        return { float(x_pos), float(y_pos) };
    }

    float InputGLFW::get_mouse_x_impl()
    {
        return get_mouse_position_impl().first;
    }

    float InputGLFW::get_mouse_y_impl()
    {
        return get_mouse_position_impl().second;
    }

}