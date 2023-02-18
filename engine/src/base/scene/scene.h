#pragma once

#include "base/scene/entity.h"
#include "base/core/timestep.h"
#include <entt/entity/registry.hpp>

namespace Yogi {

    typedef void(*SystemFunc)(const Ref<entt::registry>&);

    template<typename... Args>struct types{using type=types;};
    template<typename Sig> struct args;
    template<typename R, typename...Args>
    struct args<R(Args...)> : types<Args...>{};
    template<typename Sig> using args_t=typename args<Sig>::type;

    class Scene
    {
    public:
        Scene();
        ~Scene();

        template<typename T>
        void register_system()
        {
            register_system_on_update<T>(args_t<decltype(T::on_update)>{});
        }

        Entity create_entity();

        void on_update(Timestep ts);
    private:
        Ref<entt::registry> m_registry;
        std::vector<SystemFunc> m_system_funcs;

        template <typename T, typename... Params>
        void register_system_on_update(types<Params...>)
        {
            m_system_funcs.push_back([](const Ref<entt::registry>& registry){
                auto group = registry->group<Params...>();
                for (auto entity : group) {
                    auto args = group.get(entity);
                    std::apply(T::on_update, std::move(args));
                }
            });
        }
    };

}