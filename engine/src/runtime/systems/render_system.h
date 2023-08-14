#pragma once

#include "runtime/scene/scene.h"
#include "runtime/events/application_event.h"
#include "runtime/scene/components.h"

namespace Yogi {

    class RenderSystem
    {
    public:
        static void on_update(Timestep ts, Scene* scene);
        static void render_camera(const CameraComponent& camera, const TransformComponent& transform, Scene* scene);
        static void on_event(Event& e, Scene* scene);
        static bool on_window_resized(WindowResizeEvent& e, Scene* scene);
        static bool on_window_close(WindowCloseEvent& e, Scene* scene);
        
        static void set_default_frame_texture(const Ref<RenderTexture>& frame_texture);
    };

}