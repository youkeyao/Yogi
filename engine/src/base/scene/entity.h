#pragma once

#include <entt/entity/registry.hpp>

namespace Yogi {

    class Scene;
    class Entity
    {
        friend class Scene;
    public:
        Entity() = default;
        template<typename T, typename... Args>
        T& add_component(Args&&... args)
        {
            T& component = m_registry->emplace<T>(m_entity_handle, std::forward<Args>(args)...);
            return component;
        }

        template<typename T>
        T& get_component()
        {
            YG_CORE_ASSERT(m_registry->any_of<T>(m_entity_handle), "Entity get invalid component!");
            T& component = m_registry->get<T>(m_entity_handle);
            return component;
        }

        void each_component(std::function<void(std::string_view, void*)> func)
        {
            for (auto [id, storage] : m_registry->storage()) {
                if (storage.contains(m_entity_handle)) {
                    entt::type_info type = storage.type();
                    func(type.name(), storage.value(m_entity_handle));
                }
            }
        }

        operator bool() const { return m_entity_handle != entt::null; }
        operator uint32_t() const { return (uint32_t)m_entity_handle; }
        bool operator==(const Entity& other) const
        {
            return m_entity_handle == other.m_entity_handle && m_registry == other.m_registry;
        }
        bool operator!=(const Entity& other) const
        {
            return m_entity_handle != other.m_entity_handle || m_registry != other.m_registry;
        }
    private:
        Entity(entt::entity handle, entt::registry* registry);
        entt::entity m_entity_handle{ entt::null };
        entt::registry* m_registry = nullptr;
    };

}