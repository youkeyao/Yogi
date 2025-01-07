#pragma once

#include "runtime/scene/entity.h"
#include "runtime/scene/system_base.h"

#include <entt/entity/registry.hpp>
#include <entt/entity/runtime_view.hpp>

namespace Yogi {

class Scene
{
    typedef void (*SystemUpdateFunc)(Timestep, Scene *);
    typedef void (*SystemEventFunc)(Event &, Scene *);

public:
    Scene();
    ~Scene();

    template <typename T>
    void add_system()
    {
        std::string system_name = get_type_name<T>();
        for (auto &[name, system] : m_systems) {
            if (name == system_name) {
                return;
            }
        }
        m_systems.push_back({ system_name, CreateRef<T>() });
    }

    void add_runtime_system(const std::string &system_name, const Ref<SystemBase> &system)
    {
        for (auto &[name, sys] : m_systems) {
            if (name == system_name) {
                return;
            }
        }
        m_systems.push_back({ system_name, system });
    }

    template <typename T>
    void remove_system()
    {
        std::string system_name = get_type_name<T>();
        for (auto iter = m_systems.begin(); iter != m_systems.end(); iter++) {
            auto &[name, system] = *iter;
            if (name == system_name) {
                iter = m_systems.erase(iter);
                return;
            }
        }
    }

    void remove_runtime_system(const std::string &system_name)
    {
        for (auto iter = m_systems.begin(); iter != m_systems.end(); iter++) {
            auto &[name, system] = *iter;
            if (name == system_name) {
                iter = m_systems.erase(iter);
                return;
            }
        }
    }

    template <typename... Args, typename F = std::function<void(Args &&...)>>
    void view_components(F func)
    {
        auto view = m_registry.view<Args...>();
        for (auto entity : view) {
            Entity e(entity, &m_registry);
            std::apply([&](auto &...args) { func(e, args...); }, view.get(entity));
        }
    }
    void view_runtime_components(const std::vector<uint32_t> &components, std::function<void(Entity)> func)
    {
        entt::runtime_view view{};
        for (auto component : components) {
            entt::sparse_set *storage = m_registry.storage(component);
            if (storage) {
                view.iterate(*storage);
            }
        }
        view.each([this, func](entt::entity entity) { func(Entity{ entity, &m_registry }); });
    }

    Entity create_entity(uint32_t hint = 0);
    Entity get_entity(uint32_t handle);
    void   delete_entity(Entity entity);

    void each_entity(std::function<void(Entity)> func);
    void each_system(std::function<void(std::string)> func);
    void change_system_order(uint32_t old_index, uint32_t new_index);

    void on_update(Timestep ts);
    void on_event(Event &e);

private:
    entt::registry                                       m_registry;
    std::vector<std::pair<std::string, Ref<SystemBase>>> m_systems;
};

}  // namespace Yogi