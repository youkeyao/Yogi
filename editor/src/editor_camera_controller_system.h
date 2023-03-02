#pragma once

#include <engine.h>

namespace Yogi {

    class EditorCameraControllerSystem
    {
    public:
        static void on_update(Timestep ts, Scene* scene);
        static void on_event(Event& e, Scene* scene);
    private:
        static bool on_mouse_scrolled(MouseScrolledEvent& e, Scene* scene);
        static bool on_window_resized(WindowResizeEvent& e, Scene* scene);
    };

}