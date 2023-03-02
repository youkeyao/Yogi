#include "base/scene/scene.h"

namespace Yogi {

    Scene::Scene()
    {
    }

    Scene::~Scene()
    {
        
    }

    Entity Scene::create_entity()
    {
        entt::entity handle = m_registry.create();
        return Entity{handle, &m_registry};
    }

    void Scene::delete_entity(Entity entity)
    {
        m_registry.destroy(entity.m_entity_handle);
    }

    void Scene::each_entity(std::function<void(Entity)> func)
    {
        m_registry.each([this, func](entt::entity entity_id){
            func(Entity({entity_id, &m_registry}));
        });
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