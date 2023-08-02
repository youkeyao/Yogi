#include "reflect/system_manager.h"

namespace Yogi {
    std::unordered_map<std::string, SystemManager::SystemFunc> SystemManager::s_add_system_funcs{};
    std::unordered_map<std::string, SystemManager::SystemFunc> SystemManager::s_remove_system_funcs{};

    void SystemManager::init()
    {
        register_system<RenderSystem>();
    }

    template <typename Type>
    void SystemManager::register_system()
    {
        std::string system_name = get_type_name<Type>();
        s_add_system_funcs[system_name] = [](Ref<Scene> scene){
            scene->add_system<Type>();
        };
        s_remove_system_funcs[system_name] = [](Ref<Scene> scene){
            scene->remove_system<Type>();
        };
    }

    void SystemManager::add_system(Ref<Scene> scene, std::string system_name)
    {
        s_add_system_funcs[system_name](scene);
    }

    void SystemManager::remove_system(Ref<Scene> scene, std::string system_name)
    {
        s_remove_system_funcs[system_name](scene);
    }

    void SystemManager::each_system_type(std::function<void(std::string)> func)
    {
        for (auto [component_name, component_type] : s_add_system_funcs) {
            func(component_name);
        }
    }

}