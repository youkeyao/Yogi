#pragma once

#include <entt/entity/registry.hpp>
#include "base/scene/component_manager.h"

namespace Yogi {

    class Scene;
    class Entity
    {
        friend class Scene;
    public:
        void add_component(std::string name)
        {
            // uint32_t component_size = ComponentManager::get_component_size(name);
            // uint8_t type[component_size];
            // auto &&storage = m_registry->storage<decltype(type)>(name);
            // storage.emplace(m_entity_handle);
        }

        template<typename T>
        T& get_component()
        {
            T& component = m_registry->get<T>(m_entity_handle);
            return component;
        }

        void each_component(std::function<void(void)> func)
        {
            for (auto [id, storage] : m_registry->storage()) {
                if (storage.contains(m_entity_handle)) {
                    entt::type_info type = storage.type();
                    YG_CORE_INFO("{0}", type.name());
                }
            }
            // YG_CORE_INFO("{0}", m_registry->);
            // auto components = m_registry->ctx();
            // return components;
        }

        bool operator==(const Entity& other) const
        {
            return m_entity_handle == other.m_entity_handle && m_registry == other.m_registry;
        }
    private:
        Entity(entt::entity handle, Ref<entt::registry> registry);
        entt::entity m_entity_handle{ entt::null };
        Ref<entt::registry> m_registry;
    };

}