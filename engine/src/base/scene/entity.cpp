#include "base/scene/entity.h"
#include "base/scene/scene.h"
#include "base/scene/component_manager.h"

namespace Yogi {

    template <typename...>
    entt::registry::base_type* create_storage_impl(const std::string& name, const uint32_t& component_size, entt::registry& registry)
    {
        return nullptr;
    }
    template <std::size_t Index, std::size_t... Indices>
    entt::registry::base_type* create_storage_impl(const std::string& name, const uint32_t& component_size, entt::registry& registry)
    {
        if (component_size == Index) {
            auto& t = registry.storage<ComponentStorage<Index>>(entt::hashed_string{name.c_str()});
            return (entt::registry::base_type*)registry.storage(entt::hashed_string{name.c_str()});
        }
        return create_storage_impl<Indices...>(name, component_size, registry);
    }
    template <std::size_t... Indices>
    entt::registry::base_type* create_storage(const std::string& name, entt::registry& registry, std::index_sequence<Indices...>)
    {
        uint32_t component_size = ComponentManager::get_component_size(name);
        return create_storage_impl<Indices...>(name, component_size, registry);
    }

    Entity::Entity(entt::entity handle, Scene* scene)
        : m_entity_handle(handle), m_scene(scene)
    {}

    void Entity::add_component(std::string name)
    {
        if (m_scene->m_storages.find(name) == m_scene->m_storages.end()) {
            m_scene->m_storages[name] = create_storage(name, m_scene->m_registry, std::make_index_sequence<ComponentManager::max_component_size>{});
        }
        m_scene->m_storages[name]->push(m_entity_handle);
    }

    void* Entity::get_component(std::string name)
    {
        return m_scene->m_storages[name]->value(m_entity_handle);
    }

    void Entity::each_component(std::function<void(void)> func)
    {
        // for (auto s : m_registry->storage()) {
        //     auto [id, storage] = s;
        //     entt::sparse_set& t = s.second;
        //     if (storage.contains(m_entity_handle)) {
        //         entt::type_info type = storage.type();
        //         YG_CORE_INFO("{0}", type.name());
        //     }
        // }
        // YG_CORE_INFO("{0}", m_registry->);
        // auto components = m_registry->ctx();
        // return components;
    }
    
}