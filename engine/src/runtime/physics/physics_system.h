#pragma once

#include "runtime/events/application_event.h"
#include "runtime/scene/scene.h"
#include "runtime/scene/system_base.h"

#include <Jolt/Jolt.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSystem.h>

namespace Yogi {

class PhysicsSystem : public SystemBase
{
public:
    PhysicsSystem();
    ~PhysicsSystem();

    void on_update(Timestep ts, Scene *scene) override;
    void on_event(Event &e, Scene *scene) override;

private:
    JPH::PhysicsSystem                 *m_physics_system = nullptr;
    JPH::TempAllocatorImpl             *m_temp_allocator = nullptr;
    JPH::JobSystemThreadPool           *m_job_system = nullptr;
    JPH::BroadPhaseLayerInterface      *m_broad_phase_layer_interface = nullptr;
    JPH::ObjectVsBroadPhaseLayerFilter *m_object_vs_broadphase_layer_filter = nullptr;
    JPH::ObjectLayerPairFilter         *m_object_vs_object_layer_filter = nullptr;

    std::vector<JPH::BodyID> m_bodies;
};

}  // namespace Yogi