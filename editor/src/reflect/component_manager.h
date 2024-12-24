#pragma once

#include <engine.h>

namespace Yogi {

struct Field
{
    std::size_t offset = 0;
    std::size_t type_hash = 0;
};

struct ComponentType
{
    std::unordered_map<std::string, Field> m_fields;
};

class ComponentManager
{
    typedef void *(*AddComponentFunc)(Entity &);
    typedef void (*RemoveComponentFunc)(Entity &);

public:
    static void          init();
    static ComponentType get_component_type(std::string component_name);
    static void         *add_component(Entity &entity, std::string component_name);
    static void          remove_component(Entity &entity, std::string component_name);
    static void          each_component_type(std::function<void(std::string)> func);

    template <typename Type> static void register_component(std::vector<std::string> field_names);

private:
    static std::unordered_map<std::string, ComponentType>       s_component_types;
    static std::unordered_map<std::string, AddComponentFunc>    s_add_component_funcs;
    static std::unordered_map<std::string, RemoveComponentFunc> s_remove_component_funcs;
};

}  // namespace Yogi