#pragma once

#include "base/scene/components.h"

namespace Yogi {

    class RenderSystem
    {
    public:
        static void on_update(TransformComponent transform, SpriteRendererComponent sprite);
    };

}