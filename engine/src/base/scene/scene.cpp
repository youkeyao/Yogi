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
        for (auto func : m_system_funcs) {
            func(m_registry);
        }
    }

}