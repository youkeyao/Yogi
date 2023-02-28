#pragma once

#include <glm/glm.hpp>

namespace Yogi {

    struct Field
    {
        std::size_t type_hash = 0;
        std::size_t offset = 0;
    };

    struct ComponentType
    {
        std::size_t size = 0;
        std::unordered_map<std::string, Field> m_fields;
    };

    template <std::size_t N>
    struct ComponentStorage
    {
        uint8_t data[N];
    };

    class ComponentManager
    {
    public:
        static const std::size_t max_component_size = 50;
        static void init();
        static uint32_t get_component_size(std::string name);

        template <typename... Types>
        static void add_component_type(std::string name, std::vector<std::string> field_names)
        {
            ComponentType component_type;
            uint32_t index = 0;
            ((component_type.m_fields[field_names[index++]] = { typeid(Types).hash_code(), component_type.size },
                component_type.size += sizeof(Types)), ...);
            YG_CORE_ASSERT(component_type.size < max_component_size, "Component size exceeds!");
            m_component_types[name] = component_type;
        }

        template <typename T>
        static T& field(void* ptr, std::string type_name, std::string field_name)
        {
            Field field = m_component_types[type_name].m_fields[field_name];
            YG_CORE_ASSERT(field.type_hash == typeid(T).hash_code(), "Component type error!");
            return *(T*)((uint8_t*)ptr + field.offset);
        }
    private:
        static std::unordered_map<std::string, ComponentType> m_component_types;
    };

}