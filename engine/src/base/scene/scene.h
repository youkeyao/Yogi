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

        template<typename... Args, typename F = std::function<void(Args&&...)>>
        void view_components(F func)
        {
            auto view = m_registry->view<Args...>();
            for (auto entity : view) {
                std::apply(func, view.get(entity));
            }
        }

        void each_entity(std::function<void(Ref<Entity>)> func)
        {
            m_registry->each([this, func](entt::entity entity_id){
                Entity entity(entity_id, m_registry);
                func(CreateRef<Entity>(entity));
            });
        }

        Ref<Entity> create_entity();
        void delete_entity(Ref<Entity> entity);

        void on_update(Timestep ts);
        void on_event(Event& e);
    private:
        Ref<entt::registry> m_registry;
        std::vector<SystemUpdateFunc> m_system_update_funcs;
        std::vector<SystemEventFunc> m_system_event_funcs;
        
        template<typename T>
        auto register_on_update(T*) -> decltype(T::on_update, void())
        {
            m_system_update_funcs.push_back([](Timestep ts, Scene* scene){
                T::on_update(ts, scene);
            });
        }
        template<typename T>
        void register_on_update(...)
        {}
        template<typename T>
        auto register_on_event(T*) -> decltype(T::on_event, void())
        {
            m_system_event_funcs.push_back([](Event& e, Scene* scene){
                T::on_event(e, scene);
            });
        }
        template<typename T>
        void register_on_event(...)
        {}
    };

}