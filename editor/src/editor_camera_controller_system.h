#pragma once

#include <engine.h>

namespace Yogi {

    class EditorCameraControllerSystem
    {
    public:
        static void on_update(Timestep ts, Ref<Entity> editor_camera);
        static void on_event(Event& e, Ref<Entity> editor_camera);
    private:
        static bool on_mouse_scrolled(MouseScrolledEvent& e, Ref<Entity> editor_camera);
        static bool on_window_resized(WindowResizeEvent& e, Ref<Entity> editor_camera);
    };

}