#include "runtime/systems/physics_system.h"
#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Physics/PhysicsSystem.h>

namespace Yogi {

    JPH::PhysicsSystem s_physics_system;

    PhysicsSystem::PhysicsSystem()
    {
        JPH::Factory::sInstance = new JPH::Factory();
    }
    PhysicsSystem::~PhysicsSystem()
    {
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }

    void PhysicsSystem::on_update(Timestep ts, Scene* scene)
    {
    }

    void PhysicsSystem::on_event(Event& e, Scene* scene)
    {
    }

}