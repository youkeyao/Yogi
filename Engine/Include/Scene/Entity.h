#pragma once

#include "Core/Type.h"

#include <entt/entity/registry.hpp>

namespace Yogi
{

class World;
class YG_API Entity
{
    friend class World;

public:
    template <typename T, typename... Args>
    T& AddComponent(Args&&... args)
    {
        entt::storage<T>& pool = m_registry->storage<T>(GetTypeHash<T>());
        if (pool.contains(m_entityHandle))
        {
            return GetComponent<T>();
        }
        return *reinterpret_cast<T*>(&pool.emplace(m_entityHandle, std::forward<Args>(args)...));
    }

    template <typename T>
    void RemoveComponent()
    {
        entt::storage<T>& pool = m_registry->storage<T>(GetTypeHash<T>());
        YG_CORE_ASSERT(pool.contains(m_entityHandle), "Entity remove invalid component!");
        return pool.erase(m_entityHandle);
    }

    template <typename T>
    T& GetComponent()
    {
        entt::storage<T>& pool = m_registry->storage<T>(GetTypeHash<T>());
        YG_CORE_ASSERT(pool.contains(m_entityHandle), "Entity get invalid component!");
        return *reinterpret_cast<T*>(pool.value(m_entityHandle));
    }

    template <typename F>
    void EachComponent(F&& func)
    {
        for (auto [id, storage] : m_registry->storage())
        {
            if (storage.contains(m_entityHandle))
            {
                func(id, storage.value(m_entityHandle));
            }
        }
    }

    operator bool() const { return m_entityHandle != entt::null; }
    operator uint32_t() const { return (uint32_t)m_entityHandle; }
    bool operator==(const Entity& other) const
    {
        return m_entityHandle == other.m_entityHandle && m_registry == other.m_registry;
    }
    bool operator!=(const Entity& other) const
    {
        return m_entityHandle != other.m_entityHandle || m_registry != other.m_registry;
    }
    bool operator<(const Entity& other) const { return m_entityHandle < other.m_entityHandle; }

    static Entity Null() { return Entity(entt::null, nullptr); }

private:
    Entity(entt::entity handle, const Ref<entt::registry>& registry);
    entt::entity        m_entityHandle;
    Ref<entt::registry> m_registry;
};

} // namespace Yogi