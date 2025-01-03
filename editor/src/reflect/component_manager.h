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
    std::size_t                            m_size = 0;
};

class ComponentManager
{
    typedef void *(*AddComponentFunc)(Entity &, const std::string &);
    typedef void (*RemoveComponentFunc)(Entity &, const std::string &);

public:
    static void          init();
    static std::string   get_component_name(uint32_t component_type);
    static ComponentType get_component_type(std::string component_name);
    static void         *add_component(Entity &entity, std::string component_name);
    static void          remove_component(Entity &entity, std::string component_name);
    static void          each_component_type(std::function<void(std::string)> func);

    template <typename Type>
    static void register_component(std::vector<std::string> field_names);
    static void register_component(std::string component_name, ComponentType component_type);

private:
    static std::vector<std::function<void *(Entity &, const std::string &)>> s_add_custom_component_funcs;

    static std::unordered_map<uint32_t, std::string>            s_component_names;
    static std::unordered_map<std::string, ComponentType>       s_component_types;
    static std::unordered_map<std::string, AddComponentFunc>    s_add_component_funcs;
    static std::unordered_map<std::string, RemoveComponentFunc> s_remove_component_funcs;
};

}  // namespace Yogi