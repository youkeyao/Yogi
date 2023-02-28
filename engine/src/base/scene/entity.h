#pragma once

#include <entt/entity/registry.hpp>
#include "base/scene/component_manager.h"

namespace Yogi {

    class Scene;
    class Entity
    {
        friend class Scene;
    public:
        void add_component(std::string name);
        void* get_component(std::string name);

        void each_component(std::function<void(void)> func);

        bool operator==(const Entity& other) const
        {
            return m_entity_handle == other.m_entity_handle && m_scene == other.m_scene;
        }
    private:
        Entity(entt::entity handle, Scene* scene);
        entt::entity m_entity_handle{ entt::null };
        Scene* m_scene;
    };

}