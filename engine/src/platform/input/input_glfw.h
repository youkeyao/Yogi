#pragma once

#include "core/input.h"
#include "core/application.h"
#include "GLFW/glfw3.h"

namespace hazel {

    class InputGLFW : public Input
    {
    protected:
        bool is_key_pressed_impl(int keycode) override;
        bool is_mouse_button_pressed_impl(int button) override;
        std::pair<float, float> get_mouse_position_impl() override;
        float get_mouse_x_impl() override;
        float get_mouse_y_impl() override;
    };

}