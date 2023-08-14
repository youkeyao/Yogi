#pragma once

#include "runtime/scene/scene.h"
#include "runtime/events/application_event.h"

namespace Yogi {

    class LightSystem
    {
    public:
        static void on_update(Timestep ts, Scene* scene);
        static void on_event(Event& e, Scene* scene);
        static bool on_window_close(WindowCloseEvent& e, Scene* scene);
    };

}