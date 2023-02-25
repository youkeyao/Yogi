#pragma once

#include <glm/glm.hpp>
#include "base/renderer/texture.h"

namespace Yogi {

    // struct Field
    // {
    //     std::string name = "";
    //     uint32_t size = 0;
    //     uint32_t offset = 0;
    // };

    // struct ComponentType
    // {
    //     std::unordered_map<std::string, uint32_t> fields_name;
    // };

    // template <size_t N>
    // struct ComponentStorage
    // {
    //     uint8_t buffer[N];
    // };

    // class ComponentManager
    // {
    // public:        
    //     static void init();
    //     static void add_component_type(std::string name, ComponentType type);
    //     static uint32_t get_component_size(std::string type);

    //     template <typename T>
    //     static void set_field(std::string type, void* ptr, std::string name, T value)
    //     {
    //         Field field = m_component_types[type].fields[name];
    //         *(T*)(ptr + field.offset) = value;
    //     }
    //     template <typename T>
    //     static T& get_field(std::string type, void* ptr, std::string name)
    //     {
    //         Field field = m_component_types[type].fields[name];
    //         return *(T*)(ptr + field.offset);
    //     }
    // private:
    //     static std::unordered_map<std::string, ComponentType> m_component_types;
    // };

    // struct Field
    // {
    //     const std::type_info& type;
    //     Field(const std::type_info& type, void* value) : type(type), value(value) {}
        
    // private:
    //     const void* value;
    // };
    

    template <std::size_t Index, typename Type>
    struct ComponentUnit {
        std::string m_field_name;
        Type m_value;

        ComponentUnit(const std::string& field_name, Type value)
         : m_value(value), m_field_name(field_name)
        {}

        ~ComponentUnit() = default;
    };

    template <typename, typename...>
    struct ComponentImpl;

    template <std::size_t... Indices, typename... Types>
    struct ComponentImpl<std::index_sequence<Indices...>, Types...> : public ComponentUnit<Indices, Types>...
    {
        ComponentImpl(const std::vector<std::string>& field_names, Types... value)
         : ComponentUnit<Indices, Types>(field_names[Indices], value)...
        {}

        template <typename T>
        T& field(std::string field_name)
        {
            return *(T*)field_impl<0, Types...>(field_name);
        }

        void each_field(std::function<void(std::string, const std::type_info&)> func)
        {
            each<0, Types...>(func);
        }

        ~ComponentImpl() = default;
    private:
        template <std::size_t Index>
        void each(std::function<void(std::string, const std::type_info&)> func) {}
        template <std::size_t Index, typename T, typename... Ts>
        void each(std::function<void(std::string, const std::type_info&)> func)
        {
            using UnitType = ComponentUnit<Index, T>;
            func(static_cast<UnitType*>(this)->m_field_name, typeid(T));
            each<Index + 1, Ts...>(func);
        }
        template <std::size_t Index>
        void* field_impl(std::string field_name) { return nullptr; }
        template <std::size_t Index, typename T, typename... Ts>
        void* field_impl(std::string field_name)
        {
            using UnitType = ComponentUnit<Index, T>;
            UnitType* ptr = static_cast<UnitType*>(this);
            if (ptr->m_field_name == field_name) {
                return &ptr->m_value;
            }
            return field_impl<Index + 1, Ts...>(field_name);
        }
    };

    template <typename... Types>
    struct ComponentBase : public ComponentImpl<std::make_index_sequence<sizeof...(Types)>, Types...>
    {
        using base_type = ComponentImpl<std::make_index_sequence<sizeof...(Types)>, Types...>;
        const std::string name;

        ComponentBase(const std::string& name, const std::vector<std::string>& field_names, Types... values)
         : base_type(field_names, values...), name(name)
        {}

        ~ComponentBase() = default;
    };

    // template <typename, typename... Ts>
    // void* get_field_impl(mtuple<Ts...>& tuple, std::string field_name)
    // {
    //     return nullptr;
    // }

    // template <std::size_t Index, typename T, typename... Ts>
    // T& get_field_impl(mtuple<T, Ts...>& tuple, std::string field_name)
    // {
    //     using impl_type  = mtuple_value<Index, T>;
    //     if (field_name == static_cast<impl_type>(tuple).m_field_name) {
    //         return static_cast<impl_type&>(tuple).m_value;
    //     }
    //     return get_field_impl<Index + 1, Ts...>(tuple, field_name);
    // }

    // template <typename... Types>
    // auto& get_field(mtuple<Types...>& tuple, std::string field_name)
    // {
    //     // size_t index = std::distance(m_field_names.begin(), std::find(m_field_names.begin(), m_field_names.end(), field_name));
    //     return get_field_impl<0, Types...>(tuple, field_name);
    // }

//     template <std::size_t Index, typename... Types>
// struct mtuple_element;

// template <std::size_t Index>
// struct mtuple_element<Index> {
//     using type = void;
// };

// template <std::size_t Index, typename Head, typename... Tail>
// struct mtuple_element<Index, Head, Tail...> {
//     using impl_type = mtuple_value<Index, Head>;
//     using type = check() ? impl_type : mtuple_element(Index + 1, Tail...)::type;
//     mtuple<Head, Tail...> m_tuple;
//     std::string m_field_name;
//     mtuple_element(mtuple<Head, Tail...> tuple, std::string field_name) : m_tuple(tuple), m_field_name(field_name) {}
//     bool check()
//     {
//         return m_field_name == static_cast<impl_type>(tuple).m_field_name;
//     }
// };

// template <std::size_t Index, typename... Types>
// using mtuple_element_t = typename mtuple_element<Index, Types...>::type;

//     template <std::size_t Index, typename ...Types>
// struct mtuple_helper {
//     using type = mtuple_value<Index, mtuple_element_t<Index, Types...>>;
// };

// template <std::size_t Index, typename... Types>
// using mtuple_helper_t = typename mtuple_helper<Index, Types...>::type;

//     template <std::size_t Index, typename T, typename... ElemTypes>
// auto& get(mtuple<T, ElemTypes...>& tuple) {
//     using impl_type  = mtuple_value<Index, T>;
//     return (static_cast<impl_type&>(tuple).m_value);
// }

    // template <typename... Types>
    // struct ComponentBase
    // {
    //     const std::string name;
    //     ComponentBase(const std::string& name, const std::vector<std::string>& field_names, Types... values)
    //         : name(name), m_field_names(field_names)
    //     {
    //         m_values = std::make_tuple(values...);
    //         init(field_names, std::make_index_sequence<sizeof...(Types)>{});
    //     }

    //     template <typename T>
    //     T& field(std::string field_name)
    //     {
    //         return *(T*)m_fields[field_name];
    //     }

    //     void each_field(std::function<void(std::string, const std::type_info&)> func)
    //     {
    //         each(func, m_values);
    //     }

    // private:
    //     const std::vector<std::string> m_field_names;
    //     std::unordered_map<std::string, void*> m_fields;
    //     std::tuple<Types...> m_values;

    //     template <std::size_t... Indices>
    //     void init(const std::vector<std::string>& field_names, std::index_sequence<Indices...>)
    //     {
    //         ((m_fields[field_names[Indices]] = &std::get<Indices>(m_values)), ...);
    //     }

    //     template <typename... Ts>
    //     void each(std::function<void(std::string, const std::type_info&)> func, std::tuple<Ts...>&)
    //     {
    //         uint32_t index = 0;
    //         ((func(m_field_names[index++], typeid(Ts))), ...);
    //     }
    // };

}