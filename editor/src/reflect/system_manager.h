#pragma once

#include <engine.h>

namespace Yogi {

class SystemManager
{
    typedef void (*SystemFunc)(Ref<Scene>);

public:
    static void init();
    static void add_system(Ref<Scene> scene, std::string system_name);
    static void remove_system(Ref<Scene> scene, std::string system_name);
    static void each_system_type(std::function<void(std::string)> func);

    template <typename Type> static void register_system();

private:
    static std::unordered_map<std::string, SystemFunc> s_add_system_funcs;
    static std::unordered_map<std::string, SystemFunc> s_remove_system_funcs;
};

}  // namespace Yogi