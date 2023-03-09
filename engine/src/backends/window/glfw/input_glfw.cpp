#include "runtime/core/input.h"
#include "runtime/core/application.h"
#include "GLFW/glfw3.h"

namespace Yogi {

    bool Input::is_key_pressed(int keycode)
    {
        auto window = static_cast<GLFWwindow*>(Application::get().get_window().get_native_window());
        int state = glfwGetKey(window, keycode);
        return state == GLFW_PRESS || state == GLFW_REPEAT;
    }

    bool Input::is_mouse_button_pressed(int button)
    {
        auto window = static_cast<GLFWwindow*>(Application::get().get_window().get_native_window());
        int state = glfwGetMouseButton(window, button);
        return state == GLFW_PRESS;
    }

    std::pair<float, float> Input::get_mouse_position()
    {
        auto window = static_cast<GLFWwindow*>(Application::get().get_window().get_native_window());
        double x_pos, y_pos;
        glfwGetCursorPos(window, &x_pos, &y_pos);
        return { float(x_pos), float(y_pos) };
    }

    float Input::get_mouse_x()
    {
        return get_mouse_position().first;
    }

    float Input::get_mouse_y()
    {
        return get_mouse_position().second;
    }

}