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
        std::string systemName = GetTypeName<T>();
        for (auto& [name, system] : m_systems)
        {
            if (name == systemName)
            {
                return;
            }
        }
        m_systems.push_back({ systemName, Handle<T>::Create() });
    }

    template <typename T>
    void RemoveSystem()
    {
        std::string systemName = GetTypeName<T>();
        for (auto iter = m_systems.begin(); iter != m_systems.end(); iter++)
        {
            auto& [name, system] = *iter;
            if (name == systemName)
            {
                iter = m_systems.erase(iter);
                return;
            }
        }
    }

    template <typename... Args, typename F = std::function<void(Args&&...)>>
    void ViewComponents(F&& func)
    {
        auto view = m_registry->view<Args...>();
        for (auto entity : view)
        {
            Entity e(entity, Ref<entt::registry>::Create(m_registry));
            std::apply([&](auto&... args) { func(e, args...); }, view.get(entity));
        }
    }

    Entity CreateEntity(uint32_t hint = 0);
    Entity GetEntity(uint32_t handle);
    void   DeleteEntity(Entity entity);

    void EachEntity(std::function<void(Entity)>&& func);
    void EachSystem(std::function<void(std::string)>&& func);

    void ChangeSystemOrder(uint32_t old_index, uint32_t new_index);

    void OnUpdate(Timestep ts);
    void OnEvent(Event& e);

private:
    Handle<entt::registry>                                  m_registry;
    std::vector<std::pair<std::string, Handle<SystemBase>>> m_systems;
};

} // namespace Yogi