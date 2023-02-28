#pragma once

#include "base/scene/entity.h"
#include "base/core/timestep.h"
#include "base/events/event.h"
#include <entt/entity/registry.hpp>
#include <entt/entity/runtime_view.hpp>

namespace Yogi {

    class Scene
    {
        friend class Entity;
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

        void view_components(std::vector<std::string> component_names, std::function<void(std::vector<void*>)> func)
        {
            entt::runtime_view view{};
            for (auto component_name : component_names) {
                view.iterate(*m_storages[component_name]);
            }
            for (auto entity : view) {
                std::vector<void*> components;
                for (auto component_name : component_names) {
                    components.push_back(m_storages[component_name]->value(entity));
                }
                func(components);
            }
        }

        void each_entity(std::function<void(Ref<Entity>)> func)
        {
            m_registry.each([this, func](entt::entity entity_id){
                Entity entity(entity_id, this);
                func(CreateRef<Entity>(entity));
            });
        }

        Ref<Entity> create_entity();
        void delete_entity(Ref<Entity> entity);

        void on_update(Timestep ts);
        void on_event(Event& e);
    private:
        entt::registry m_registry;
        std::unordered_map<std::string, entt::registry::base_type*> m_storages;
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