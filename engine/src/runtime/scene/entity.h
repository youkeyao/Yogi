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
            if (m_registry->any_of<T>(m_entity_handle)) {
                return get_component<T>();
            }
            T& component = m_registry->emplace<T>(m_entity_handle, std::forward<Args>(args)...);
            return component;
        }

        template<typename T>
        void remove_component()
        {
            YG_CORE_ASSERT(m_registry->any_of<T>(m_entity_handle), "Entity remove invalid component!");
            m_registry->erase<T>(m_entity_handle);
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
                    std::string_view name = type.name();
                    size_t pos = name.find(' ');
                    if (pos != std::string_view::npos) {
                        name = name.substr(pos + 1);
                    }
                    func(name, storage.value(m_entity_handle));
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