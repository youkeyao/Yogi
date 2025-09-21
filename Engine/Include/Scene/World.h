#pragma once

#include "Scene/Entity.h"
#include "Scene/SystemBase.h"

#include <entt/entity/runtime_view.hpp>

namespace Yogi
{

class YG_API World
{
public:
    World();
    ~World();

    template <typename T>
    void AddSystem()
    {
        uint32_t systemHash = GetTypeHash<T>();
        for (auto& [hash, system] : m_systems)
        {
            if (hash == systemHash)
            {
                return;
            }
        }
        m_systems.push_back({ systemHash, Handle<T>::Create() });
    }

    template <typename T>
    void RemoveSystem()
    {
        uint32_t systemHash = GetTypeHash<T>();
        for (auto iter = m_systems.begin(); iter != m_systems.end(); iter++)
        {
            auto& [hash, system] = *iter;
            if (hash == systemHash)
            {
                iter = m_systems.erase(iter);
                return;
            }
        }
    }

    template <typename... Args, typename F = std::function<void(Args&&...)>>
    void ViewComponents(F&& func)
    {
        entt::view<entt::get_t<Args...>> view;
        ((view.storage(m_registry->storage<Args>(GetTypeHash<Args>()))), ...);
        view.each([this, func](entt::entity entity, Args&... args) {
            Entity e(entity, Ref<entt::registry>::Create(m_registry));
            func(e, args...);
        });
    }

    Entity CreateEntity(uint32_t hint = 0);
    Entity GetEntity(uint32_t handle);
    void   DeleteEntity(Entity entity);

    void EachEntity(std::function<void(Entity)>&& func);
    void EachSystem(std::function<void(uint32_t)>&& func);

    void ChangeSystemOrder(uint32_t old_index, uint32_t new_index);

    void OnUpdate(Timestep ts);
    void OnEvent(Event& e);

private:
    Handle<entt::registry>                               m_registry;
    std::vector<std::pair<uint32_t, Handle<SystemBase>>> m_systems;
};

} // namespace Yogi