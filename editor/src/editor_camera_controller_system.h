#pragma once

#include <engine.h>

namespace Yogi {

    class EditorCameraControllerSystem
    {
    public:
        static void on_update(Timestep ts, const Ref<Entity>& editor_camera);
        static void on_event(Event& e, const Ref<Entity>& editor_camera);
    private:
        static bool on_mouse_scrolled(MouseScrolledEvent& e, const Ref<Entity>& editor_camera);
        static bool on_window_resized(WindowResizeEvent& e, const Ref<Entity>& editor_camera);
    };

}