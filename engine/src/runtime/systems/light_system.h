#pragma once

#include "runtime/scene/scene.h"

namespace Yogi {

    class LightSystem
    {
    public:
        static void on_update(Timestep ts, Scene* scene);
    };

}