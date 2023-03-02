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

    template <typename Type>
    auto get_component_name()
    {
        std::string pretty_function{__PRETTY_FUNCTION__};
        auto first = pretty_function.find_first_not_of(' ', pretty_function.find_first_of('=') + 1);
        auto value = pretty_function.substr(first, pretty_function.find_last_of(']') - first);
        return value;
    }

    //--------------------------------------------------------------------------
    std::unordered_map<std::string, ComponentType> ComponentManager::m_component_types{};
    std::unordered_map<std::string, ComponentManager::AddComponentFunc> ComponentManager::m_add_component_funcs{};
    std::unordered_map<std::string, ComponentManager::RemoveComponentFunc> ComponentManager::m_remove_component_funcs{};

    void ComponentManager::init()
    {
        register_component<TagComponent>({"tag"});
        register_component<TransformComponent>({"parent", "translation", "rotation", "scale"});
        register_component<SpriteRendererComponent>({"color", "texture", "texcoords_min", "texcoords_max"});
        register_component<CameraComponent>({"is_ortho", "is_primary", "fov", "aspect_ratio", "zoom_level"});
    }

    template <typename Type>
    void ComponentManager::register_component(std::vector<std::string> field_names)
    {
        ComponentType component_type;
        get_fields<Type>(component_type, field_names);
        std::string component_name = get_component_name<Type>();
        m_component_types[component_name] = component_type;
        m_add_component_funcs[component_name] = [](Entity& entity){
            entity.add_component<Type>();
        };
        m_remove_component_funcs[component_name] = [](Entity& entity){
            entity.remove_component<Type>();
        };
    }

    ComponentType ComponentManager::get_component_type(std::string component_name)
    {
        YG_CORE_ASSERT(m_component_types.find(component_name) != m_component_types.end(), "Unknown component type!");
        return m_component_types[component_name];
    }

    void ComponentManager::add_component(Entity& entity, std::string component_name)
    {
        YG_CORE_ASSERT(m_component_types.find(component_name) != m_component_types.end(), "Unknown component type!");
        m_add_component_funcs[component_name](entity);
    }

    void ComponentManager::remove_component(Entity& entity, std::string component_name)
    {
        YG_CORE_ASSERT(m_component_types.find(component_name) != m_component_types.end(), "Unknown component type!");
        m_remove_component_funcs[component_name](entity);
    }

    void ComponentManager::each_component_type(std::function<void(std::string)> func)
    {
        for (auto [component_name, component_type] : m_component_types) {
            func(component_name);
        }
    }

}