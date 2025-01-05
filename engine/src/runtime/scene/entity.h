#pragma once

#include <entt/entity/registry.hpp>

namespace Yogi {

class Scene;
class Entity
{
    friend class Scene;

public:
    Entity() = default;

    template <typename T, typename... Args>
    T &add_component(Args &&...args)
    {
        if (m_registry->any_of<T>(m_entity_handle)) {
            return get_component<T>();
        }
        T &component = m_registry->emplace<T>(m_entity_handle, std::forward<Args>(args)...);
        return component;
    }

    template <typename T>
    void *add_runtime_component(uint32_t component_typeid)
    {
        entt::storage<T> &pool = m_registry->storage<T>(component_typeid);
        if (pool.contains(m_entity_handle)) {
            return get_runtime_component(component_typeid);
        }
        return (void *)&pool.emplace(m_entity_handle);
    }

    template <typename T>
    void remove_component()
    {
        YG_CORE_ASSERT(m_registry->any_of<T>(m_entity_handle), "Entity remove invalid component!");
        m_registry->erase<T>(m_entity_handle);
    }

    void remove_runtime_component(uint32_t component_typeid)
    {
        entt::sparse_set *pool = m_registry->storage(component_typeid);
        YG_CORE_ASSERT(pool->contains(m_entity_handle), "Entity remove invalid component!");
        return pool->erase(m_entity_handle);
    }

    template <typename T>
    T &get_component()
    {
        YG_CORE_ASSERT(m_registry->any_of<T>(m_entity_handle), "Entity get invalid component!");
        T &component = m_registry->get<T>(m_entity_handle);
        return component;
    }

    void *get_runtime_component(uint32_t component_typeid)
    {
        entt::sparse_set *pool = m_registry->storage(component_typeid);
        YG_CORE_ASSERT(pool->contains(m_entity_handle), "Entity get invalid component!");
        return pool->value(m_entity_handle);
    }

    template <typename F>
    void each_component(F func)
    {
        for (auto [id, storage] : m_registry->storage()) {
            if (storage.contains(m_entity_handle)) {
                func(id, storage.value(m_entity_handle));
            }
        }
    }

    operator bool() const { return m_entity_handle != entt::null; }
    operator uint32_t() const { return (uint32_t)m_entity_handle; }
    bool operator==(const Entity &other) const
    {
        return m_entity_handle == other.m_entity_handle && m_registry == other.m_registry;
    }
    bool operator!=(const Entity &other) const
    {
        return m_entity_handle != other.m_entity_handle || m_registry != other.m_registry;
    }
    bool operator<(const Entity &other) const { return m_entity_handle < other.m_entity_handle; }

private:
    Entity(entt::entity handle, entt::registry *registry);
    entt::entity    m_entity_handle{ entt::null };
    entt::registry *m_registry = nullptr;
};

}  // namespace Yogi