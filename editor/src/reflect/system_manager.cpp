#include "reflect/system_manager.h"

namespace Yogi {

std::unordered_map<std::string, SystemManager::SystemFunc> SystemManager::s_add_system_funcs{};
std::unordered_map<std::string, SystemManager::SystemFunc> SystemManager::s_remove_system_funcs{};

std::unordered_map<std::string, Ref<SystemManager::RuntimeSystem>> SystemManager::s_runtime_systems{};

void SystemManager::init()
{
    register_system<RenderSystem>();
    register_system<PhysicsSystem>();
}
void SystemManager::clear()
{
    s_add_system_funcs.clear();
    s_remove_system_funcs.clear();
    s_runtime_systems.clear();
}

void SystemManager::add_system(const Ref<Scene> &scene, const std::string &system_name)
{
    s_add_system_funcs[system_name](scene, system_name);
}

void SystemManager::remove_system(const Ref<Scene> &scene, const std::string &system_name)
{
    s_remove_system_funcs[system_name](scene, system_name);
}

void SystemManager::each_system_type(std::function<void(std::string)> func)
{
    for (auto [component_name, component_type] : s_add_system_funcs) {
        func(component_name);
    }
}

}  // namespace Yogi