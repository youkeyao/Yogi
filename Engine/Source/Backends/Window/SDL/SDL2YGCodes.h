#pragma once

#include "runtime/events/key_codes.h"
#include "runtime/events/mouse_button_codes.h"

#include <SDL.h>

namespace Yogi
{

uint16_t sdl_to_yg_codes(uint16_t sdl_codes)
{
    switch (sdl_codes)
    {
        case SDL_BUTTON_LEFT: return YG_MOUSE_BUTTON_LEFT;
        case SDL_BUTTON_MIDDLE: return YG_MOUSE_BUTTON_MIDDLE;
        case SDL_BUTTON_RIGHT: return YG_MOUSE_BUTTON_RIGHT;
        case SDL_SCANCODE_A: return YG_KEY_A;
        case SDL_SCANCODE_B: return YG_KEY_B;
        case SDL_SCANCODE_C: return YG_KEY_C;
        case SDL_SCANCODE_D: return YG_KEY_D;
        case SDL_SCANCODE_E: return YG_KEY_E;
        case SDL_SCANCODE_F: return YG_KEY_F;
        case SDL_SCANCODE_G: return YG_KEY_G;
        case SDL_SCANCODE_H: return YG_KEY_H;
        case SDL_SCANCODE_I: return YG_KEY_I;
        case SDL_SCANCODE_J: return YG_KEY_J;
        case SDL_SCANCODE_K: return YG_KEY_K;
        case SDL_SCANCODE_L: return YG_KEY_L;
        case SDL_SCANCODE_M: return YG_KEY_M;
        case SDL_SCANCODE_N: return YG_KEY_N;
        case SDL_SCANCODE_O: return YG_KEY_O;
        case SDL_SCANCODE_P: return YG_KEY_P;
        case SDL_SCANCODE_Q: return YG_KEY_Q;
        case SDL_SCANCODE_R: return YG_KEY_R;
        case SDL_SCANCODE_S: return YG_KEY_S;
        case SDL_SCANCODE_T: return YG_KEY_T;
        case SDL_SCANCODE_U: return YG_KEY_U;
        case SDL_SCANCODE_V: return YG_KEY_V;
        case SDL_SCANCODE_W: return YG_KEY_W;
        case SDL_SCANCODE_X: return YG_KEY_X;
        case SDL_SCANCODE_Y: return YG_KEY_Y;
        case SDL_SCANCODE_Z: return YG_KEY_Z;
        case SDL_SCANCODE_1: return YG_KEY_1;
        case SDL_SCANCODE_2: return YG_KEY_2;
        case SDL_SCANCODE_3: return YG_KEY_3;
        case SDL_SCANCODE_4: return YG_KEY_4;
        case SDL_SCANCODE_5: return YG_KEY_5;
        case SDL_SCANCODE_6: return YG_KEY_6;
        case SDL_SCANCODE_7: return YG_KEY_7;
        case SDL_SCANCODE_8: return YG_KEY_8;
        case SDL_SCANCODE_9: return YG_KEY_9;
        case SDL_SCANCODE_0: return YG_KEY_0;
        case SDL_SCANCODE_RETURN: return YG_KEY_ENTER;
        case SDL_SCANCODE_ESCAPE: return YG_KEY_ESCAPE;
        case SDL_SCANCODE_BACKSPACE: return YG_KEY_BACKSPACE;
        case SDL_SCANCODE_TAB: return YG_KEY_TAB;
        case SDL_SCANCODE_SPACE: return YG_KEY_SPACE;
        case SDL_SCANCODE_MINUS: return YG_KEY_MINUS;
        case SDL_SCANCODE_EQUALS: return YG_KEY_EQUAL;
        case SDL_SCANCODE_LEFTBRACKET: return YG_KEY_LEFT_BRACKET;
        case SDL_SCANCODE_RIGHTBRACKET: return YG_KEY_RIGHT_BRACKET;
        case SDL_SCANCODE_BACKSLASH: return YG_KEY_BACKSLASH;
        case SDL_SCANCODE_SEMICOLON: return YG_KEY_SEMICOLON;
        case SDL_SCANCODE_APOSTROPHE: return YG_KEY_APOSTROPHE;
        case SDL_SCANCODE_GRAVE: return YG_KEY_GRAVE_ACCENT;
        case SDL_SCANCODE_COMMA: return YG_KEY_COMMA;
        case SDL_SCANCODE_PERIOD: return YG_KEY_PERIOD;
        case SDL_SCANCODE_SLASH: return YG_KEY_SLASH;
        case SDL_SCANCODE_CAPSLOCK: return YG_KEY_CAPS_LOCK;
        case SDL_SCANCODE_F1: return YG_KEY_F1;
        case SDL_SCANCODE_F2: return YG_KEY_F2;
        case SDL_SCANCODE_F3: return YG_KEY_F3;
        case SDL_SCANCODE_F4: return YG_KEY_F4;
        case SDL_SCANCODE_F5: return YG_KEY_F5;
        case SDL_SCANCODE_F6: return YG_KEY_F6;
        case SDL_SCANCODE_F7: return YG_KEY_F7;
        case SDL_SCANCODE_F8: return YG_KEY_F8;
        case SDL_SCANCODE_F9: return YG_KEY_F9;
        case SDL_SCANCODE_F10: return YG_KEY_F10;
        case SDL_SCANCODE_F11: return YG_KEY_F11;
        case SDL_SCANCODE_F12: return YG_KEY_F12;
        case SDL_SCANCODE_PRINTSCREEN: return YG_KEY_PRINT_SCREEN;
        case SDL_SCANCODE_SCROLLLOCK: return YG_KEY_SCROLL_LOCK;
        case SDL_SCANCODE_PAUSE: return YG_KEY_PAUSE;
        case SDL_SCANCODE_INSERT: return YG_KEY_INSERT;
        case SDL_SCANCODE_HOME: return YG_KEY_HOME;
        case SDL_SCANCODE_PAGEUP: return YG_KEY_PAGE_UP;
        case SDL_SCANCODE_DELETE: return YG_KEY_DELETE;
        case SDL_SCANCODE_END: return YG_KEY_END;
        case SDL_SCANCODE_PAGEDOWN: return YG_KEY_PAGE_DOWN;
        case SDL_SCANCODE_RIGHT: return YG_KEY_RIGHT;
        case SDL_SCANCODE_LEFT: return YG_KEY_LEFT;
        case SDL_SCANCODE_DOWN: return YG_KEY_DOWN;
        case SDL_SCANCODE_UP: return YG_KEY_UP;
        case SDL_SCANCODE_NUMLOCKCLEAR: return YG_KEY_NUM_LOCK;
        case SDL_SCANCODE_KP_DIVIDE: return YG_KEY_KP_DIVIDE;
        case SDL_SCANCODE_KP_MULTIPLY: return YG_KEY_KP_MULTIPLY;
        case SDL_SCANCODE_KP_MINUS: return YG_KEY_KP_SUBTRACT;
        case SDL_SCANCODE_KP_PLUS: return YG_KEY_KP_ADD;
        case SDL_SCANCODE_KP_ENTER: return YG_KEY_KP_ENTER;
        case SDL_SCANCODE_KP_1: return YG_KEY_KP_1;
        case SDL_SCANCODE_KP_2: return YG_KEY_KP_2;
        case SDL_SCANCODE_KP_3: return YG_KEY_KP_3;
        case SDL_SCANCODE_KP_4: return YG_KEY_KP_4;
        case SDL_SCANCODE_KP_5: return YG_KEY_KP_5;
        case SDL_SCANCODE_KP_6: return YG_KEY_KP_6;
        case SDL_SCANCODE_KP_7: return YG_KEY_KP_7;
        case SDL_SCANCODE_KP_8: return YG_KEY_KP_8;
        case SDL_SCANCODE_KP_9: return YG_KEY_KP_9;
        case SDL_SCANCODE_KP_0: return YG_KEY_KP_0;
        case SDL_SCANCODE_KP_EQUALS: return YG_KEY_KP_EQUAL;
        case SDL_SCANCODE_MENU: return YG_KEY_MENU;
        case SDL_SCANCODE_LCTRL: return YG_KEY_LEFT_CONTROL;
        case SDL_SCANCODE_LSHIFT: return YG_KEY_LEFT_SHIFT;
        case SDL_SCANCODE_LALT: return YG_KEY_LEFT_ALT;
        case SDL_SCANCODE_RCTRL: return YG_KEY_RIGHT_CONTROL;
        case SDL_SCANCODE_RSHIFT: return YG_KEY_RIGHT_SHIFT;
        case SDL_SCANCODE_RALT: return YG_KEY_RIGHT_ALT;
    }
    YG_CORE_ASSERT(false, "Unknown codes!");
    return 0;
}

} // namespace Yogi