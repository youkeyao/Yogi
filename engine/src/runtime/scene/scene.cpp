#include "runtime/scene/scene.h"

namespace Yogi {

Scene::Scene() {}

Scene::~Scene() {
    m_systems.clear();
    m_registry.clear();
}

Entity Scene::create_entity(uint32_t hint)
{
    YG_PROFILE_SCOPE("create_entity");
    entt::entity handle = m_registry.create((entt::entity)hint);
    return Entity{ handle, &m_registry };
}

Entity Scene::get_entity(uint32_t handle)
{
    if (m_registry.orphan((entt::entity)handle))
        return Entity{};
    return Entity{ (entt::entity)handle, &m_registry };
}

void Scene::delete_entity(Entity entity)
{
    m_registry.destroy(entity.m_entity_handle);
}

void Scene::each_entity(std::function<void(Entity)> func)
{
    m_registry.each([this, func](entt::entity entity_id) { func(Entity({ entity_id, &m_registry })); });
}

void Scene::each_system(std::function<void(std::string)> func)
{
    for (int32_t i = 0; i < m_systems.size(); i++) {
        func(m_systems[i].first);
    }
}

void Scene::change_system_order(uint32_t old_index, uint32_t new_index)
{
    YG_CORE_ASSERT(
        0 <= old_index && old_index < m_systems.size() && 0 <= new_index && new_index < m_systems.size(),
        "Invalid system order!");
    std::swap(m_systems[old_index], m_systems[new_index]);
}

void Scene::on_update(Timestep ts)
{
    YG_PROFILE_FUNCTION();

    for (int32_t i = 0; i < m_systems.size(); i++) {
        m_systems[i].second->on_update(ts, *this);
    }
}

void Scene::on_event(Event &e)
{
    YG_PROFILE_FUNCTION();

    for (int32_t i = 0; i < m_systems.size(); i++) {
        m_systems[i].second->on_event(e, *this);
    }
}

}  // namespace Yogi