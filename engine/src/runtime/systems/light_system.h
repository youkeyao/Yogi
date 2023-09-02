#pragma once

#include "runtime/scene/scene.h"
#include "runtime/systems/system_base.h"
#include "runtime/events/application_event.h"
#include "runtime/renderer/frame_buffer.h"

namespace Yogi {

    class LightSystem : public SystemBase
    {
    public:
        LightSystem();
        ~LightSystem();

        void on_update(Timestep ts, Scene* scene) override;
        void on_event(Event& e, Scene* scene) override;
    private:
        int m_shadow_map_size = 2048;
        Ref<FrameBuffer> m_shadow_frame_buffer;
    };

}