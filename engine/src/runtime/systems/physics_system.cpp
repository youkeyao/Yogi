#include "runtime/systems/physics_system.h"
#include "runtime/scene/components.h"
#include <Jolt/Core/Factory.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

namespace Yogi {

    const uint32_t cMaxBodies = 65536;
	const uint32_t cNumBodyMutexes = 0;
	const uint32_t cMaxBodyPairs = 1024;
    const uint32_t cMaxContactConstraints = 1024;

    namespace Layers
    {
        static constexpr JPH::ObjectLayer NON_MOVING = 0;
        static constexpr JPH::ObjectLayer MOVING = 1;
        static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
    };
    namespace BroadPhaseLayers
    {
        static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
        static constexpr JPH::BroadPhaseLayer MOVING(1);
        static constexpr uint32_t NUM_LAYERS(2);
    };
    class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
    {
    public:
        BPLayerInterfaceImpl()
        {
            mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
            mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
        }

        virtual uint32_t GetNumBroadPhaseLayers() const override
        {
            return BroadPhaseLayers::NUM_LAYERS;
        }

        virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
        {
            YG_CORE_ASSERT(inLayer < Layers::NUM_LAYERS, "Jolt BroadPhaseLayer Error!");
            return mObjectToBroadPhase[inLayer];
        }

        virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
        {
            switch ((JPH::BroadPhaseLayer::Type)inLayer)
            {
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING: return "NON_MOVING";
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING: return "MOVING";
            default: YG_CORE_ASSERT(false, "Invalid BroadPhaseLayer!"); return "INVALID";
            }
        }
    private:
        JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
    };
    class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
    {
    public:
        virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
        {
            switch (inLayer1)
            {
            case Layers::NON_MOVING:
                return inLayer2 == BroadPhaseLayers::MOVING;
            case Layers::MOVING:
                return true;
            default:
                YG_CORE_ASSERT(false, "Jolt ObjectVsBroadPhaseLayerFilter Error!");
                return false;
            }
        }
    };
    class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
    {
    public:
        virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
        {
            switch (inObject1)
            {
            case Layers::NON_MOVING:
                return inObject2 == Layers::MOVING;
            case Layers::MOVING:
                return true;
            default:
                YG_CORE_ASSERT(false, "Jolt ObjectLayerPairFilter Error!");
                return false;
            }
        }
    };

    PhysicsSystem::PhysicsSystem()
    {
        JPH::RegisterDefaultAllocator();
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();

        m_temp_allocator = new JPH::TempAllocatorImpl(16 * 1024 * 1024);
        m_job_system = new JPH::JobSystemThreadPool(1024, 8, 4);

        m_broad_phase_layer_interface = new BPLayerInterfaceImpl();
        m_object_vs_broadphase_layer_filter = new ObjectVsBroadPhaseLayerFilterImpl();
        m_object_vs_object_layer_filter = new ObjectLayerPairFilterImpl();
        m_physics_system.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, *m_broad_phase_layer_interface, *m_object_vs_broadphase_layer_filter, *m_object_vs_object_layer_filter);
    }
    PhysicsSystem::~PhysicsSystem()
    {
        if (!m_bodies.empty()) {
            m_physics_system.GetBodyInterface().RemoveBodies(m_bodies.data(), m_bodies.size());
            m_physics_system.GetBodyInterface().DestroyBodies(m_bodies.data(), m_bodies.size());
        }

        delete m_broad_phase_layer_interface;
        delete m_object_vs_broadphase_layer_filter;
        delete m_object_vs_object_layer_filter;

        delete m_temp_allocator;
        delete m_job_system;

        JPH::UnregisterTypes();

        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }

    void PhysicsSystem::on_update(Timestep ts, Scene* scene)
    {
        JPH::BodyInterface& body_interface = m_physics_system.GetBodyInterface();

        // add body || update body transform
        scene->view_components<TransformComponent, RigidBodyComponent>([&](Entity entity, TransformComponent& transform, RigidBodyComponent& rigid_body){
            JPH::BodyID body_id(entity);
            glm::mat4 transform_mat = transform.transform;
            JPH::Mat44 transform_matrix(
                JPH::Vec4(transform_mat[0][0], transform_mat[0][1], transform_mat[0][2], transform_mat[0][3]),
                JPH::Vec4(transform_mat[1][0], transform_mat[1][1], transform_mat[1][2], transform_mat[1][3]),
                JPH::Vec4(transform_mat[2][3], transform_mat[2][1], transform_mat[2][2], transform_mat[2][3]),
                JPH::Vec4(transform_mat[3][0], transform_mat[3][1], transform_mat[3][2], transform_mat[3][3])
            );
            JPH::Vec3 scale;
            JPH::Mat44 decomposed_matrix = transform_matrix.Decompose(scale);
            JPH::Vec3 translation = decomposed_matrix.GetTranslation();
            decomposed_matrix.SetTranslation(JPH::Vec3(0, 0, 0));
            JPH::Quat rotation = decomposed_matrix.GetQuaternion();

            JPH::ShapeSettings::ShapeResult shape_result;
            if (rigid_body.type == ColliderType::BOX) {
                JPH::BoxShapeSettings shape_settings(JPH::Vec3(rigid_body.scale.x, rigid_body.scale.y, rigid_body.scale.z) * scale / 2);
                shape_result = shape_settings.Create();
            }
            else if (rigid_body.type == ColliderType::SPHERE) {
                JPH::SphereShapeSettings shape_settings((rigid_body.scale.x + rigid_body.scale.y + rigid_body.scale.z) / 3 * (scale.GetX() + scale.GetY() + scale.GetZ()) / 3);
                shape_result = shape_settings.Create();
            }
            else if (rigid_body.type == ColliderType::CAPSULE) {
                JPH::CapsuleShapeSettings shape_settings(rigid_body.scale.y * scale.GetY() / 2, (rigid_body.scale.x + rigid_body.scale.z) / 2 * (scale.GetX() + scale.GetZ()) / 2 / 2);
                shape_result = shape_settings.Create();
            }

            if (!body_interface.IsAdded(body_id)) {
                JPH::BodyCreationSettings body_create_settings(
                    shape_result.Get(),
                    translation,
                    rotation,
                    rigid_body.is_static ? JPH::EMotionType::Static : JPH::EMotionType::Dynamic,
                    rigid_body.is_static ? Layers::NON_MOVING : Layers::MOVING
                );
                JPH::Body* body = body_interface.CreateBodyWithID(JPH::BodyID(entity), body_create_settings);
                body_interface.AddBody(body->GetID(), rigid_body.is_static ? JPH::EActivation::DontActivate : JPH::EActivation::Activate);
                m_bodies.push_back(body->GetID());
            }
            else {
                body_interface.SetPosition(body_id, translation, JPH::EActivation::Activate);
                body_interface.SetRotation(body_id, rotation, JPH::EActivation::Activate);
                body_interface.SetShape(body_id, shape_result.Get(), true, JPH::EActivation::Activate);
            }
        });

        m_physics_system.Update(ts, (int)(ts * 60) + 1, m_temp_allocator, m_job_system);

        // update entity transform
        scene->view_components<TransformComponent, RigidBodyComponent>([&](Entity entity, TransformComponent& transform, RigidBodyComponent& rigid_body){
            JPH::Mat44 transform_matrix = body_interface.GetWorldTransform(JPH::BodyID(entity));
            glm::vec3 scale;
            glm::mat4 tf = transform.transform;
            scale.x = glm::length(glm::vec3{tf[0][0], tf[0][1], tf[0][2]});
            scale.y = glm::length(glm::vec3{tf[1][0], tf[1][1], tf[1][2]});
            scale.z = glm::length(glm::vec3{tf[2][0], tf[2][1], tf[2][2]});
            transform.transform = glm::mat4(
                transform_matrix(0, 0) * scale.x, transform_matrix(1, 0) * scale.x, transform_matrix(2, 0) * scale.x, transform_matrix(3, 0),
                transform_matrix(0, 1) * scale.y, transform_matrix(1, 1) * scale.y, transform_matrix(2, 1) * scale.y, transform_matrix(3, 1),
                transform_matrix(0, 2) * scale.z, transform_matrix(1, 2) * scale.z, transform_matrix(2, 2) * scale.z, transform_matrix(3, 2),
                transform_matrix(0, 3), transform_matrix(1, 3), transform_matrix(2, 3), transform_matrix(3, 3)
            );
        });
    }

    void PhysicsSystem::on_event(Event& e, Scene* scene)
    {
    }

}