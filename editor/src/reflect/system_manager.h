#pragma once

#include <engine.h>

namespace Yogi {

    class SystemManager
    {
        typedef void(*AddSystemFunc)(Ref<Scene>);
    public:
        static void init();
        static void add_system(Ref<Scene> scene, std::string system_name);

        template <typename Type>
        static void register_system();
    private:
        static std::unordered_map<std::string, AddSystemFunc> s_add_system_funcs;
    };

}