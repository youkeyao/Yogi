#pragma once

#include "runtime/scene/entity.h"
#include "runtime/systems/system_base.h"
#include <entt/entity/registry.hpp>

namespace Yogi {

    class Scene
    {
        typedef void(*SystemUpdateFunc)(Timestep, Scene*);
        typedef void(*SystemEventFunc)(Event&, Scene*);
    public:
        Scene();
        ~Scene();

        template<typename T>
        void add_system()
        {
            std::string system_name = get_type_name<T>();
            for (auto& [name, system] : m_systems) {
                if (name == system_name) {
                    return;
                }
            }
            m_systems.push_back({system_name, CreateRef<T>()});
        }

        template<typename T>
        void remove_system()
        {
            std::string system_name = get_type_name<T>();
            for (auto iter = m_systems.begin(); iter != m_systems.end(); iter++) {
                auto& [name, system] = *iter;
                if (name == system_name) {
                    iter = m_systems.erase(iter);
                    if (iter == m_systems.end()) break;
                }
            }
        }

        template<typename... Args, typename F = std::function<void(Args&&...)>>
        void view_components(F func)
        {
            auto view = m_registry.view<Args...>();
            for (auto entity : view) {
                Entity e(entity, &m_registry);
                std::apply([&](auto&... args){func(e, args...);}, view.get(entity));
            }
        }

        Entity create_entity(uint32_t hint = 0);
        Entity get_entity(uint32_t handle);
        void delete_entity(Entity entity);

        void each_entity(std::function<void(Entity)> func);
        void each_system(std::function<void(std::string)> func);
        void change_system_order(uint32_t old_index, uint32_t new_index);

        void on_update(Timestep ts);
        void on_event(Event& e);
    private:
        entt::registry m_registry;
        std::vector<std::pair<std::string, Ref<SystemBase>>> m_systems;
    };

}