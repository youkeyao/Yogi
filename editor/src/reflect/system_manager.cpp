#include "reflect/system_manager.h"

namespace Yogi {
    std::unordered_map<std::string, SystemManager::AddSystemFunc> SystemManager::s_add_system_funcs{};

    void SystemManager::init()
    {
        register_system<RenderSystem>();
        register_system<CameraSystem>();
    }

    template <typename Type>
    void SystemManager::register_system()
    {
        std::string system_name = get_type_name<Type>();
        s_add_system_funcs[system_name] = [](Ref<Scene> scene){
            scene->register_system<Type>();
        };
    }

    void SystemManager::add_system(Ref<Scene> scene, std::string system_name)
    {
        s_add_system_funcs[system_name](scene);
    }

}