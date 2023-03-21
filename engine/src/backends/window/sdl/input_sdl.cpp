#include "runtime/core/input.h"
#include "runtime/core/application.h"
#include "backends/window/sdl/yg_to_sdl_codes.h"
#include <SDL.h>

namespace Yogi {

    bool Input::is_key_pressed(int keycode)
    {
        const uint8_t* states = SDL_GetKeyboardState(nullptr);
        return states[yg_to_sdl_codes(keycode)];
    }

    bool Input::is_mouse_button_pressed(int button)
    {
        uint32_t states = SDL_GetMouseState(nullptr, nullptr);
        return states & SDL_BUTTON(yg_to_sdl_codes(button));
    }

    std::pair<float, float> Input::get_mouse_position()
    {
        int32_t x_pos, y_pos;
        SDL_GetMouseState(&x_pos, &y_pos);
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