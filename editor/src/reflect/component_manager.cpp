#include "reflect/component_manager.h"

namespace Yogi {

    struct Any
    {
        template <typename T>
        operator T();
        template <typename T, typename = std::enable_if_t<!std::is_lvalue_reference<T>::value>>
        operator T&&() const;
        template <typename T, typename = std::enable_if_t<std::is_copy_constructible<T>::value>>
        operator T&() const;
    };

    template<typename T, typename... Args>
    constexpr auto member_count(T* t) -> decltype(T{Args{}..., Any{}}, std::size_t{})
    {
        return member_count<T, Args..., Any>(t);
    }
    template<typename T, typename... Args>
    constexpr auto member_count(...)
    {
        return sizeof...(Args);
    }

    template <typename... Types>
    constexpr void add_fields(ComponentType& type, const std::vector<std::string>& field_names, std::size_t base, Types&&... fields)
    {
        uint32_t index = 0;
        ((type.m_fields[field_names[index++]] = {
            (std::size_t)&fields - base,
            typeid(Types).hash_code()
        }), ...);
    }

    template <typename T>
    constexpr void get_fields(ComponentType& type, const std::vector<std::string>& field_names)
    {
        T object{};
        constexpr auto Count = member_count<T>(0);
        YG_CORE_ASSERT(Count == field_names.size(), "Component reflect error!");
        if constexpr (Count == 1) {
            auto &&[a1] = object;
            add_fields(type, field_names, (std::size_t)&object, a1);
        }
        else if constexpr (Count == 2) {
            auto &&[a1, a2] = object;
            add_fields(type, field_names, (std::size_t)&object, a1, a2);
        }
        else if constexpr (Count == 3) {
            auto &&[a1, a2, a3] = object;
            add_fields(type, field_names, (std::size_t)&object, a1, a2, a3);
        }
        else if constexpr (Count == 4) {
            auto &&[a1, a2, a3, a4] = object;
            add_fields(type, field_names, (std::size_t)&object, a1, a2, a3, a4);
        }
        else if constexpr (Count == 5) {
            auto &&[a1, a2, a3, a4, a5] = object;
            add_fields(type, field_names, (std::size_t)&object, a1, a2, a3, a4, a5);
        }
    }

    //--------------------------------------------------------------------------
    std::unordered_map<std::string, ComponentType> ComponentManager::s_component_types{};
    std::unordered_map<std::string, ComponentManager::AddComponentFunc> ComponentManager::s_add_component_funcs{};
    std::unordered_map<std::string, ComponentManager::RemoveComponentFunc> ComponentManager::s_remove_component_funcs{};

    void ComponentManager::init()
    {
        register_component<TagComponent>({"tag"});
        register_component<TransformComponent>({"parent", "transform"});
        register_component<SpriteRendererComponent>({"color", "texture", "tex_min", "tex_max"});
        register_component<CameraComponent>({"is_ortho", "is_primary", "fov", "aspect_ratio", "zoom_level"});
    }

    template <typename Type>
    void ComponentManager::register_component(std::vector<std::string> field_names)
    {
        ComponentType component_type;
        get_fields<Type>(component_type, field_names);
        std::string component_name = get_type_name<Type>();
        s_component_types[component_name] = component_type;
        s_add_component_funcs[component_name] = [](Entity& entity)->void*{
            return (void*)&entity.add_component<Type>();
        };
        s_remove_component_funcs[component_name] = [](Entity& entity){
            entity.remove_component<Type>();
        };
    }

    ComponentType ComponentManager::get_component_type(std::string component_name)
    {
        YG_CORE_ASSERT(s_component_types.find(component_name) != s_component_types.end(), "Unknown component type!");
        return s_component_types[component_name];
    }

    void* ComponentManager::add_component(Entity& entity, std::string component_name)
    {
        YG_CORE_ASSERT(s_component_types.find(component_name) != s_component_types.end(), "Unknown component type!");
        return s_add_component_funcs[component_name](entity);
    }

    void ComponentManager::remove_component(Entity& entity, std::string component_name)
    {
        YG_CORE_ASSERT(s_component_types.find(component_name) != s_component_types.end(), "Unknown component type!");
        s_remove_component_funcs[component_name](entity);
    }

    void ComponentManager::each_component_type(std::function<void(std::string)> func)
    {
        for (auto [component_name, component_type] : s_component_types) {
            func(component_name);
        }
    }

}