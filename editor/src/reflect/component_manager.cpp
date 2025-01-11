#include "reflect/component_manager.h"

namespace Yogi {

struct Any
{
    template <typename T>
    operator T();
    template <typename T, typename = std::enable_if_t<!std::is_lvalue_reference<T>::value>>
    operator T &&() const;
    template <typename T, typename = std::enable_if_t<std::is_copy_constructible<T>::value>>
    operator T &() const;
};

template <typename T, typename... Args>
constexpr auto member_count(T *t) -> decltype(T{ Args{}..., Any{} }, std::size_t{})
{
    return member_count<T, Args..., Any>(t);
}
template <typename T, typename... Args>
constexpr auto member_count(...)
{
    return sizeof...(Args);
}

template <typename... Types>
constexpr void add_fields(ComponentType &type, const std::vector<std::string> &field_names, std::size_t base, Types &&...fields)
{
    uint32_t index = 0;
    ((type.fields[field_names[index++]] = { (std::size_t)&fields - base, typeid(Types).hash_code() }), ...);
}

template <typename T>
constexpr void get_fields(ComponentType &type, const std::vector<std::string> &field_names)
{
    T object{};

    constexpr auto Count = member_count<T>(0);
    YG_CORE_ASSERT(Count == field_names.size() && Count <= 6, "Component reflect error!");
    if constexpr (Count == 1) {
        auto &&[a1] = object;
        add_fields(type, field_names, (std::size_t)&object, a1);
    } else if constexpr (Count == 2) {
        auto &&[a1, a2] = object;
        add_fields(type, field_names, (std::size_t)&object, a1, a2);
    } else if constexpr (Count == 3) {
        auto &&[a1, a2, a3] = object;
        add_fields(type, field_names, (std::size_t)&object, a1, a2, a3);
    } else if constexpr (Count == 4) {
        auto &&[a1, a2, a3, a4] = object;
        add_fields(type, field_names, (std::size_t)&object, a1, a2, a3, a4);
    } else if constexpr (Count == 5) {
        auto &&[a1, a2, a3, a4, a5] = object;
        add_fields(type, field_names, (std::size_t)&object, a1, a2, a3, a4, a5);
    } else if constexpr (Count == 6) {
        auto &&[a1, a2, a3, a4, a5, a6] = object;
        add_fields(type, field_names, (std::size_t)&object, a1, a2, a3, a4, a5, a6);
    }
    type.size = sizeof(T);
}

template <size_t N>
struct TypeN
{
    char data[N];
};

template <std::size_t... Is>
constexpr void register_runtime_component(
    std::vector<std::function<void *(Entity &, uint32_t)>> &add_funcs, std::integer_sequence<std::size_t, Is...>)
{
    int expand[] = { (
        add_funcs.emplace_back([](Entity &entity, uint32_t component_typeid) -> void * {
            return entity.add_runtime_component<TypeN<Is + 1>>(component_typeid);
        }),
        0)... };
    (void)expand;
}

//--------------------------------------------------------------------------
std::unordered_map<uint32_t, std::string>                              ComponentManager::s_component_names{};
std::unordered_map<std::string, ComponentType>                         ComponentManager::s_component_types{};
std::unordered_map<std::string, ComponentManager::AddComponentFunc>    ComponentManager::s_add_component_funcs{};
std::unordered_map<std::string, ComponentManager::RemoveComponentFunc> ComponentManager::s_remove_component_funcs{};
std::vector<std::function<void *(Entity &, uint32_t)>>                 ComponentManager::s_add_runtime_component_funcs{};

void ComponentManager::init()
{
    register_component<TagComponent>({ "tag" });
    register_component<TransformComponent>({ "parent", "transform" });
    register_component<MeshRendererComponent>({ "mesh", "material", "cast_shadow" });
    register_component<CameraComponent>({ "is_ortho", "fov", "aspect_ratio", "zoom_level" });
    register_component<DirectionalLightComponent>({ "color" });
    register_component<SpotLightComponent>({ "inner_angle", "outer_angle", "color" });
    register_component<PointLightComponent>({ "attenuation_parms", "color" });
    register_component<SkyboxComponent>({ "material" });
    register_component<RigidBodyComponent>({ "is_static", "scale", "type" });
    register_runtime_component(
        s_add_runtime_component_funcs, std::make_integer_sequence<std::size_t, RUMTIME_COMPONENT_MAX_SIZE>());
}
void ComponentManager::clear()
{
    s_component_names.clear();
    s_component_types.clear();
    s_add_component_funcs.clear();
    s_remove_component_funcs.clear();
    s_add_runtime_component_funcs.clear();
}

template <typename Type>
void ComponentManager::register_component(std::vector<std::string> field_names)
{
    ComponentType component_type;
    get_fields<Type>(component_type, field_names);
    std::string component_name = get_type_name<Type>();
    component_type.type_hash = entt::type_hash<Type>::value();
    s_component_names[entt::type_hash<Type>::value()] = component_name;
    s_component_types[component_name] = component_type;
    s_add_component_funcs[component_name] = [](Entity &entity, const std::string &component_name) -> void * {
        return (void *)&entity.add_component<Type>();
    };
    s_remove_component_funcs[component_name] = [](Entity &entity, const std::string &component_name) {
        entity.remove_component<Type>();
    };
}

void ComponentManager::register_component(std::string component_name, ComponentType component_type)
{
    YG_ASSERT(component_type.size <= RUMTIME_COMPONENT_MAX_SIZE, "Component size too large!");
    component_type.type_hash = entt::hashed_string::value(component_name.c_str());
    s_component_names[component_type.type_hash] = component_name;
    s_component_types[component_name] = component_type;
    s_add_component_funcs[component_name] = [](Entity &entity, const std::string &component_name) -> void * {
        ComponentType &component_type = s_component_types[component_name];
        return s_add_runtime_component_funcs[component_type.size - 1](entity, component_type.type_hash);
    };
    s_remove_component_funcs[component_name] = [](Entity &entity, const std::string &component_name) {
        ComponentType &component_type = s_component_types[component_name];
        entity.remove_runtime_component(component_type.type_hash);
    };
}

std::string ComponentManager::get_component_name(uint32_t component_type)
{
    YG_CORE_ASSERT(s_component_names.find(component_type) != s_component_names.end(), "Get unknown component name!");
    return s_component_names[component_type];
}

const ComponentType &ComponentManager::get_component_type(std::string component_name)
{
    YG_CORE_ASSERT(s_component_types.find(component_name) != s_component_types.end(), "Get unknown component type!");
    return s_component_types[component_name];
}

void *ComponentManager::add_component(Entity &entity, std::string component_name)
{
    YG_CORE_ASSERT(s_component_types.find(component_name) != s_component_types.end(), "Add unknown component type!");
    return s_add_component_funcs[component_name](entity, component_name);
}

void ComponentManager::remove_component(Entity &entity, std::string component_name)
{
    YG_CORE_ASSERT(s_component_types.find(component_name) != s_component_types.end(), "Remove unknown component type!");
    s_remove_component_funcs[component_name](entity, component_name);
}

void ComponentManager::each_component_type(std::function<void(std::string)> func)
{
    for (auto [component_name, component_type] : s_component_types) {
        func(component_name);
    }
}

}  // namespace Yogi