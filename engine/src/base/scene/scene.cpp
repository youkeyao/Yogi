#include "base/scene/scene.h"

namespace Yogi {

    Scene::Scene()
    {
        
    }

    Scene::~Scene()
    {
        
    }

    Ref<Entity> Scene::create_entity()
    {
        entt::entity handle = m_registry.create();
        Entity entity(handle, this);
        return CreateRef<Entity>(entity);
    }

    void Scene::delete_entity(Ref<Entity> entity)
    {
        m_registry.destroy(entity->m_entity_handle);
    }

    void Scene::on_update(Timestep ts)
    {
        YG_PROFILE_FUNCTION();

        for (auto func : m_system_update_funcs) {
            func(ts, this);
        }
    }

    void Scene::on_event(Event& e)
    {
        YG_PROFILE_FUNCTION();
        
        for (auto func : m_system_event_funcs) {
            func(e, this);
        }
    }

}