#include "runtime/scene/scene.h"

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
        YG_CORE_ASSERT(0 <= old_index && old_index < m_systems.size() && 0 <= new_index && new_index < m_systems.size(), "Invalid system order!");
        std::pair<std::string, std::pair<int32_t, int32_t>> system = m_systems[old_index];
        int32_t& update_index = system.second.first;
        int32_t& event_index = system.second.second;
        if (update_index >= 0) {
            SystemUpdateFunc func = m_system_update_funcs[update_index];
            int32_t dir = old_index < new_index ? 1 : -1;
            int32_t update_new_index = update_index;
            for (int32_t id = old_index + dir; id != new_index + dir; id += dir) {
                int32_t& update_id = m_systems[id].second.first;
                if (update_id >= 0) {
                    update_new_index = update_id;
                    update_id -= dir;
                }
            }
            if (update_index != update_new_index) {
                m_system_update_funcs.erase(m_system_update_funcs.begin() + update_index);
                m_system_update_funcs.insert(m_system_update_funcs.begin() + update_new_index, func);
                update_index = update_new_index;
            }
        }
        if (event_index >= 0) {
            SystemEventFunc func = m_system_event_funcs[update_index];
            int32_t dir = old_index < new_index ? 1 : -1;
            int32_t event_new_index = event_index;
            for (int32_t id = old_index + dir; id != new_index + dir; id += dir) {
                int32_t& event_id = m_systems[id].second.second;
                if (event_id >= 0) {
                    event_new_index = event_id;
                    event_id -= dir;
                }
            }
            if (event_index != event_new_index) {
                m_system_event_funcs.erase(m_system_event_funcs.begin() + event_index);
                m_system_event_funcs.insert(m_system_event_funcs.begin() + event_new_index, func);
                event_index = event_new_index;
            }
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