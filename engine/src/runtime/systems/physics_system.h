#pragma once

#include "runtime/scene/scene.h"
#include "runtime/systems/system_base.h"
#include "runtime/events/application_event.h"

namespace Yogi {

    class PhysicsSystem : public SystemBase
    {
    public:
        PhysicsSystem();
        ~PhysicsSystem();

        void on_update(Timestep ts, Scene* scene) override;
        void on_event(Event& e, Scene* scene) override;
    };

}