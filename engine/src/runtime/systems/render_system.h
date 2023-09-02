#pragma once

#include "runtime/scene/scene.h"
#include "runtime/systems/system_base.h"
#include "runtime/events/application_event.h"
#include "runtime/scene/components.h"
#include "runtime/renderer/frame_buffer.h"

namespace Yogi {

    class RenderSystem : public SystemBase
    {
    public:
        RenderSystem();
        ~RenderSystem();

        void on_update(Timestep ts, Scene* scene) override;
        void render_camera(const CameraComponent& camera, const TransformComponent& transform, Scene* scene);

        void on_event(Event& e, Scene* scene) override;
        bool on_window_resized(WindowResizeEvent& e, Scene* scene);
        
        static void set_default_frame_buffer(const Ref<FrameBuffer>& frame_buffer);
    private:
        static int s_width;
        static int s_height;
        static FrameBuffer* s_frame_buffer;
    };

}