#include "base/scene/scene.h"

namespace Yogi {

    Scene::Scene()
    {
        m_registry = CreateRef<entt::registry>();
    }

    Scene::~Scene()
    {
        
    }

    Entity Scene::create_entity()
    {
        entt::entity handle = m_registry->create();
        Entity entity(handle, m_registry);
        return entity;
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