#pragma once

#include "base/scene/scene.h"

namespace Yogi {

    class CameraSystem
    {
    public:
        static void on_update(Timestep ts, Scene* scene);
    };

}