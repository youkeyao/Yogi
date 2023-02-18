#pragma once

#include <entt/entity/registry.hpp>

namespace Yogi {

    class Scene;
    class Entity
    {
        friend class Scene;
    public:
        template<typename T, typename... Args>
        T& add_component(Args&&... args)
        {
            T& component = m_registry->emplace<T>(m_entity_handle, std::forward<Args>(args)...);
            return component;
        }

        template<typename T>
        T& get_component()
        {
            T& component = m_registry->get<T>(m_entity_handle);
            return component;
        }
    private:
        Entity(entt::entity handle, const Ref<entt::registry>& registry);
        entt::entity m_entity_handle{ entt::null };
        Ref<entt::registry> m_registry;
    };

}