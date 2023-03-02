#pragma once

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
    public:
        static void init();
        static ComponentType get_component_type(std::string name);

        template <typename Type>
        static void register_component(std::vector<std::string> field_names);
    private:
        static std::unordered_map<std::string, ComponentType> m_component_types;
    };

}