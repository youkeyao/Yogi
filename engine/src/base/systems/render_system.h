#pragma once

#include "base/scene/scene.h"

namespace Yogi {

    class RenderSystem
    {
    public:
        static void on_update(Timestep ts, Scene* scene);
    };

}