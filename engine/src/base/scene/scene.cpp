#include "base/scene/scene.h"

namespace Yogi {

    Scene::Scene()
    {
    }

    Scene::~Scene()
    {
        
    }

    Entity Scene::create_entity(uint32_t hint)
    {
        entt::entity handle = m_registry.create((entt::entity)hint);
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

    void Scene::each_system(std::function<void(std::string, int32_t, int32_t)> func)
    {
        auto systems = m_systems;
        for (auto& [name, pos] : systems) {
            func(name, pos.first, pos.second);
        }
    }

    void Scene::change_system_order(uint32_t old_index, uint32_t new_index)
    {
        auto system = m_systems[old_index];
        int32_t update_index = system.second.first;
        int32_t event_index = system.second.second;
        if (update_index >= 0) {
            SystemUpdateFunc func = m_system_update_funcs[update_index];
            m_system_update_funcs.erase(m_system_update_funcs.begin() + update_index);
            int32_t update_new_index = m_systems[new_index].second.first;
            uint32_t system_pos = new_index;
            while (update_new_index < 0 && system_pos != m_systems.size()) {
                update_new_index = m_systems[++system_pos].second.first;
            }
            if (system_pos == m_systems.size()) update_new_index = m_system_update_funcs.size();
            m_system_update_funcs.insert(m_system_update_funcs.begin() + update_new_index, func);
            system.second.first = update_new_index;
        }
        if (event_index >= 0) {
            SystemEventFunc func = m_system_event_funcs[event_index];
            m_system_event_funcs.erase(m_system_event_funcs.begin() + event_index);
            int32_t event_new_index = m_systems[new_index].second.second;
            uint32_t system_pos = new_index;
            while (event_new_index < 0 && system_pos != m_systems.size()) {
                event_new_index = m_systems[++system_pos].second.second;
            }
            if (system_pos == m_systems.size()) event_new_index = m_system_event_funcs.size();
            m_system_event_funcs.insert(m_system_event_funcs.begin() + event_new_index, func);
            system.second.second = event_new_index;
        }
        m_systems.erase(m_systems.begin() + old_index);
        m_systems.insert(m_systems.begin() + new_index, system);
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