#pragma once

#include "runtime/scene/scene.h"

namespace Yogi {

    class CameraSystem
    {
    public:
        static void on_update(Timestep ts, Scene* scene);
        static void on_event(Event& e, Scene* scene);
    };

}