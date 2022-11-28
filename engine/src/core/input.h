#pragma once

namespace hazel {

    class Input
    {
    public:
        static bool is_key_pressed(int keycode) { return ms_instance->is_key_pressed_impl(keycode); }
        static bool is_mouse_button_pressed(int button) {
            return ms_instance->is_mouse_button_pressed_impl(button);
        }
        static std::pair<float, float> get_mouse_position() { return ms_instance->get_mouse_position_impl(); }
        static float get_mouse_x() { return ms_instance->get_mouse_x_impl(); }
        static float get_mouse_y() { return ms_instance->get_mouse_y_impl(); }
    protected:
        virtual bool is_key_pressed_impl(int keycode) = 0;
        virtual bool is_mouse_button_pressed_impl(int button) = 0;
        virtual std::pair<float, float> get_mouse_position_impl() = 0;
        virtual float get_mouse_x_impl() = 0;
        virtual float get_mouse_y_impl() = 0;
    private:
        static Input* ms_instance;
    };

}