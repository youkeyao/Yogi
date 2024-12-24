#pragma once

#include "runtime/events/key_codes.h"
#include "runtime/events/mouse_button_codes.h"

namespace Yogi {

class Input
{
public:
    static bool                    is_key_pressed(int keycode);
    static bool                    is_mouse_button_pressed(int button);
    static std::pair<float, float> get_mouse_position();
    static float                   get_mouse_x();
    static float                   get_mouse_y();
};

}  // namespace Yogi