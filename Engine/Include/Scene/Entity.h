#pragma once

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
        if (m_registry->any_of<T>(m_entityHandle))
        {
            return GetComponent<T>();
        }
        T& component = m_registry->emplace<T>(m_entityHandle, std::forward<Args>(args)...);
        return component;
    }

    template <typename T>
    void RemoveComponent()
    {
        YG_CORE_ASSERT(m_registry->any_of<T>(m_entityHandle), "Entity remove invalid component!");
        m_registry->erase<T>(m_entityHandle);
    }

    template <typename T>
    T& GetComponent()
    {
        YG_CORE_ASSERT(m_registry->any_of<T>(m_entityHandle), "Entity get invalid component!");
        T& component = m_registry->get<T>(m_entityHandle);
        return component;
    }

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
    Entity(entt::entity handle, View<entt::registry> registry);
    entt::entity         m_entityHandle{ entt::null };
    View<entt::registry> m_registry = nullptr;
};

} // namespace Yogi