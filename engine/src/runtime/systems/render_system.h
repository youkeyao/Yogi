#pragma once

#include "runtime/scene/scene.h"

namespace Yogi {

    class RenderSystem
    {
    public:
        static void on_update(Timestep ts, Scene* scene);
        static void on_event(Event& e, Scene* scene);
    };

}