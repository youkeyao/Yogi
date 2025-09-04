#pragma once

#include "runtime/events/key_codes.h"
#include "runtime/events/mouse_button_codes.h"

#include <SDL.h>

namespace Yogi
{

uint16_t yg_to_sdl_codes(uint16_t yg_codes)
{
    switch (yg_codes)
    {
        case YG_MOUSE_BUTTON_LEFT: return SDL_BUTTON_LEFT;
        case YG_MOUSE_BUTTON_MIDDLE: return SDL_BUTTON_MIDDLE;
        case YG_MOUSE_BUTTON_RIGHT: return SDL_BUTTON_RIGHT;
        case YG_KEY_A: return SDL_SCANCODE_A;
        case YG_KEY_B: return SDL_SCANCODE_B;
        case YG_KEY_C: return SDL_SCANCODE_C;
        case YG_KEY_D: return SDL_SCANCODE_D;
        case YG_KEY_E: return SDL_SCANCODE_E;
        case YG_KEY_F: return SDL_SCANCODE_F;
        case YG_KEY_G: return SDL_SCANCODE_G;
        case YG_KEY_H: return SDL_SCANCODE_H;
        case YG_KEY_I: return SDL_SCANCODE_I;
        case YG_KEY_J: return SDL_SCANCODE_J;
        case YG_KEY_K: return SDL_SCANCODE_K;
        case YG_KEY_L: return SDL_SCANCODE_L;
        case YG_KEY_M: return SDL_SCANCODE_M;
        case YG_KEY_N: return SDL_SCANCODE_N;
        case YG_KEY_O: return SDL_SCANCODE_O;
        case YG_KEY_P: return SDL_SCANCODE_P;
        case YG_KEY_Q: return SDL_SCANCODE_Q;
        case YG_KEY_R: return SDL_SCANCODE_R;
        case YG_KEY_S: return SDL_SCANCODE_S;
        case YG_KEY_T: return SDL_SCANCODE_T;
        case YG_KEY_U: return SDL_SCANCODE_U;
        case YG_KEY_V: return SDL_SCANCODE_V;
        case YG_KEY_W: return SDL_SCANCODE_W;
        case YG_KEY_X: return SDL_SCANCODE_X;
        case YG_KEY_Y: return SDL_SCANCODE_Y;
        case YG_KEY_Z: return SDL_SCANCODE_Z;
        case YG_KEY_1: return SDL_SCANCODE_1;
        case YG_KEY_2: return SDL_SCANCODE_2;
        case YG_KEY_3: return SDL_SCANCODE_3;
        case YG_KEY_4: return SDL_SCANCODE_4;
        case YG_KEY_5: return SDL_SCANCODE_5;
        case YG_KEY_6: return SDL_SCANCODE_6;
        case YG_KEY_7: return SDL_SCANCODE_7;
        case YG_KEY_8: return SDL_SCANCODE_8;
        case YG_KEY_9: return SDL_SCANCODE_9;
        case YG_KEY_0: return SDL_SCANCODE_0;
        case YG_KEY_ENTER: return SDL_SCANCODE_RETURN;
        case YG_KEY_ESCAPE: return SDL_SCANCODE_ESCAPE;
        case YG_KEY_BACKSPACE: return SDL_SCANCODE_BACKSPACE;
        case YG_KEY_TAB: return SDL_SCANCODE_TAB;
        case YG_KEY_SPACE: return SDL_SCANCODE_SPACE;
        case YG_KEY_MINUS: return SDL_SCANCODE_MINUS;
        case YG_KEY_EQUAL: return SDL_SCANCODE_EQUALS;
        case YG_KEY_LEFT_BRACKET: return SDL_SCANCODE_LEFTBRACKET;
        case YG_KEY_RIGHT_BRACKET: return SDL_SCANCODE_RIGHTBRACKET;
        case YG_KEY_BACKSLASH: return SDL_SCANCODE_BACKSLASH;
        case YG_KEY_SEMICOLON: return SDL_SCANCODE_SEMICOLON;
        case YG_KEY_APOSTROPHE: return SDL_SCANCODE_APOSTROPHE;
        case YG_KEY_GRAVE_ACCENT: return SDL_SCANCODE_GRAVE;
        case YG_KEY_COMMA: return SDL_SCANCODE_COMMA;
        case YG_KEY_PERIOD: return SDL_SCANCODE_PERIOD;
        case YG_KEY_SLASH: return SDL_SCANCODE_SLASH;
        case YG_KEY_CAPS_LOCK: return SDL_SCANCODE_CAPSLOCK;
        case YG_KEY_F1: return SDL_SCANCODE_F1;
        case YG_KEY_F2: return SDL_SCANCODE_F2;
        case YG_KEY_F3: return SDL_SCANCODE_F3;
        case YG_KEY_F4: return SDL_SCANCODE_F4;
        case YG_KEY_F5: return SDL_SCANCODE_F5;
        case YG_KEY_F6: return SDL_SCANCODE_F6;
        case YG_KEY_F7: return SDL_SCANCODE_F7;
        case YG_KEY_F8: return SDL_SCANCODE_F8;
        case YG_KEY_F9: return SDL_SCANCODE_F9;
        case YG_KEY_F10: return SDL_SCANCODE_F10;
        case YG_KEY_F11: return SDL_SCANCODE_F11;
        case YG_KEY_F12: return SDL_SCANCODE_F12;
        case YG_KEY_PRINT_SCREEN: return SDL_SCANCODE_PRINTSCREEN;
        case YG_KEY_SCROLL_LOCK: return SDL_SCANCODE_SCROLLLOCK;
        case YG_KEY_PAUSE: return SDL_SCANCODE_PAUSE;
        case YG_KEY_INSERT: return SDL_SCANCODE_INSERT;
        case YG_KEY_HOME: return SDL_SCANCODE_HOME;
        case YG_KEY_PAGE_UP: return SDL_SCANCODE_PAGEUP;
        case YG_KEY_DELETE: return SDL_SCANCODE_DELETE;
        case YG_KEY_END: return SDL_SCANCODE_END;
        case YG_KEY_PAGE_DOWN: return SDL_SCANCODE_PAGEDOWN;
        case YG_KEY_RIGHT: return SDL_SCANCODE_RIGHT;
        case YG_KEY_LEFT: return SDL_SCANCODE_LEFT;
        case YG_KEY_DOWN: return SDL_SCANCODE_DOWN;
        case YG_KEY_UP: return SDL_SCANCODE_UP;
        case YG_KEY_NUM_LOCK: return SDL_SCANCODE_NUMLOCKCLEAR;
        case YG_KEY_KP_DIVIDE: return SDL_SCANCODE_KP_DIVIDE;
        case YG_KEY_KP_MULTIPLY: return SDL_SCANCODE_KP_MULTIPLY;
        case YG_KEY_KP_SUBTRACT: return SDL_SCANCODE_KP_MINUS;
        case YG_KEY_KP_ADD: return SDL_SCANCODE_KP_PLUS;
        case YG_KEY_KP_ENTER: return SDL_SCANCODE_KP_ENTER;
        case YG_KEY_KP_1: return SDL_SCANCODE_KP_1;
        case YG_KEY_KP_2: return SDL_SCANCODE_KP_2;
        case YG_KEY_KP_3: return SDL_SCANCODE_KP_3;
        case YG_KEY_KP_4: return SDL_SCANCODE_KP_4;
        case YG_KEY_KP_5: return SDL_SCANCODE_KP_5;
        case YG_KEY_KP_6: return SDL_SCANCODE_KP_6;
        case YG_KEY_KP_7: return SDL_SCANCODE_KP_7;
        case YG_KEY_KP_8: return SDL_SCANCODE_KP_8;
        case YG_KEY_KP_9: return SDL_SCANCODE_KP_9;
        case YG_KEY_KP_0: return SDL_SCANCODE_KP_0;
        case YG_KEY_KP_EQUAL: return SDL_SCANCODE_KP_EQUALS;
        case YG_KEY_MENU: return SDL_SCANCODE_MENU;
        case YG_KEY_LEFT_CONTROL: return SDL_SCANCODE_LCTRL;
        case YG_KEY_LEFT_SHIFT: return SDL_SCANCODE_LSHIFT;
        case YG_KEY_LEFT_ALT: return SDL_SCANCODE_LALT;
        case YG_KEY_RIGHT_CONTROL: return SDL_SCANCODE_RCTRL;
        case YG_KEY_RIGHT_SHIFT: return SDL_SCANCODE_RSHIFT;
        case YG_KEY_RIGHT_ALT: return SDL_SCANCODE_RALT;
    }
    YG_CORE_ASSERT(false, "Unknown codes!");
    return 0;
}

} // namespace Yogi