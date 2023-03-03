#pragma once

#include "base/scene/entity.h"
#include "base/core/timestep.h"
#include "base/events/event.h"
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
        void register_system()
        {
            register_on_update<T>(0);
            register_on_event<T>(0);
        }

        void register_update_func(SystemUpdateFunc func) { m_system_update_funcs.push_back(func); }
        void register_on_event_func(SystemEventFunc func) { m_system_event_funcs.push_back(func); }

        template<typename... Args, typename F = std::function<void(Args&&...)>>
        void view_components(F func)
        {
            auto view = m_registry.view<Args...>();
            for (auto entity : view) {
                std::apply(func, view.get(entity));
            }
        }

        Entity create_entity(uint32_t hint = 0);
        Entity get_entity(uint32_t handle) { return Entity{(entt::entity)handle, &m_registry}; }
        void delete_entity(Entity entity);
        void each_entity(std::function<void(Entity)> func);

        void on_update(Timestep ts);
        void on_event(Event& e);
    private:
        entt::registry m_registry;
        std::vector<SystemUpdateFunc> m_system_update_funcs;
        std::vector<SystemEventFunc> m_system_event_funcs;
        
        template<typename T>
        constexpr auto register_on_update(T*) -> decltype(T::on_update, void())
        {
            m_system_update_funcs.push_back(T::on_update);
        }
        template<typename T>
        constexpr void register_on_update(...)
        {}
        template<typename T>
        constexpr auto register_on_event(T*) -> decltype(T::on_event, void())
        {
            m_system_event_funcs.push_back(T::on_event);
        }
        template<typename T>
        constexpr void register_on_event(...)
        {}
    };

}