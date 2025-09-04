#pragma once

#include "Events/KeyCodes.h"
#include "Events/MouseButtonCodes.h"

namespace Yogi
{

class YG_API Input
{
public:
    static bool                    IsKeyPressed(int keycode);
    static bool                    IsMouseButtonPressed(int button);
    static std::pair<float, float> GetMousePosition();
    static float                   GetMouseX();
    static float                   GetMouseY();
};

} // namespace Yogi