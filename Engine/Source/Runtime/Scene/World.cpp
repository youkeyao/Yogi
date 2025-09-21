#include "Scene/World.h"

namespace Yogi
{

World::World() : m_registry(nullptr) { m_registry = Handle<entt::registry>::Create(); }

World::~World() { m_systems.clear(); }

Entity World::CreateEntity(uint32_t hint)
{
    YG_PROFILE_SCOPE("create_entity");
    entt::entity handle = m_registry->create((entt::entity)hint);
    return Entity{ handle, Ref<entt::registry>::Create(m_registry) };
}

Entity World::GetEntity(uint32_t handle)
{
    if (m_registry->orphan((entt::entity)handle))
        return Entity::Null();
    return Entity{ (entt::entity)handle, Ref<entt::registry>::Create(m_registry) };
}

void World::DeleteEntity(Entity entity) { m_registry->destroy(entity.m_entityHandle); }

void World::EachEntity(std::function<void(Entity)>&& func)
{
    for (auto entityID : m_registry->view<entt::entity>())
    {
        func(Entity({ entityID, Ref<entt::registry>::Create(m_registry) }));
    }
}

void World::EachSystem(std::function<void(uint32_t)>&& func)
{
    for (int32_t i = 0; i < m_systems.size(); i++)
    {
        func(m_systems[i].first);
    }
}

void World::ChangeSystemOrder(uint32_t oldIndex, uint32_t newIndex)
{
    YG_CORE_ASSERT(0 <= oldIndex && oldIndex < m_systems.size() && 0 <= newIndex && newIndex < m_systems.size(),
                   "Invalid system order!");
    std::swap(m_systems[oldIndex], m_systems[newIndex]);
}

void World::OnUpdate(Timestep ts)
{
    YG_PROFILE_FUNCTION();

    for (int32_t i = 0; i < m_systems.size(); i++)
    {
        m_systems[i].second->OnUpdate(ts, *this);
    }
}

void World::OnEvent(Event& e)
{
    YG_PROFILE_FUNCTION();

    for (int32_t i = 0; i < m_systems.size(); i++)
    {
        m_systems[i].second->OnEvent(e, *this);
    }
}

} // namespace Yogi