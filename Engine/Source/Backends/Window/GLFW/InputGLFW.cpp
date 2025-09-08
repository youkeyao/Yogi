#include "Core/Application.h"
#include "Core/Input.h"

#include <GLFW/glfw3.h>

namespace Yogi
{

bool Input::IsKeyPressed(int keycode)
{
    auto window = static_cast<GLFWwindow*>(Application::GetInstance().GetWindow()->GetNativeWindow());
    int  state  = glfwGetKey(window, keycode);
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool Input::IsMouseButtonPressed(int button)
{
    auto window = static_cast<GLFWwindow*>(Application::GetInstance().GetWindow()->GetNativeWindow());
    int  state  = glfwGetMouseButton(window, button);
    return state == GLFW_PRESS;
}

std::pair<float, float> Input::GetMousePosition()
{
    auto   window = static_cast<GLFWwindow*>(Application::GetInstance().GetWindow()->GetNativeWindow());
    double x_pos, y_pos;
    glfwGetCursorPos(window, &x_pos, &y_pos);
    return { float(x_pos), float(y_pos) };
}

float Input::GetMouseX() { return GetMousePosition().first; }

float Input::GetMouseY() { return GetMousePosition().second; }

} // namespace Yogi