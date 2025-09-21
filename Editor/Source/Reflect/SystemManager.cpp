#include "Reflect/SystemManager.h"

namespace Yogi
{

std::unordered_map<uint32_t, SystemManager::SystemInfo> SystemManager::s_systemInfos;

void SystemManager::Init() { RegisterSystem<ForwardRenderSystem>(); }

void SystemManager::Clear() { s_systemInfos.clear(); }

template <typename T>
void SystemManager::RegisterSystem()
{
    SystemInfo systemInfo;
    systemInfo.Name     = GetTypeName<T>();
    systemInfo.TypeHash = GetTypeHash<T>();
    if (s_systemInfos.find(systemInfo.TypeHash) != s_systemInfos.end())
        return;

    systemInfo.AddFunc = [](World& world) {
        world.AddSystem<T>();
    };
    systemInfo.RemoveFunc = [](World& world) {
        world.RemoveSystem<T>();
    };

    s_systemInfos.insert({ systemInfo.TypeHash, systemInfo });
}

void SystemManager::EachSystemType(std::function<void(const std::string&, uint32_t)>&& func)
{
    for (auto& [name, info] : s_systemInfos)
        func(info.Name, info.TypeHash);
}

void SystemManager::AddSystem(World& world, uint32_t typeHash)
{
    if (s_systemInfos.find(typeHash) != s_systemInfos.end())
        s_systemInfos[typeHash].AddFunc(world);
}

void SystemManager::RemoveSystem(World& world, uint32_t typeHash)
{
    if (s_systemInfos.find(typeHash) != s_systemInfos.end())
        s_systemInfos[typeHash].RemoveFunc(world);
}

std::string SystemManager::GetSystemName(uint32_t typeHash)
{
    if (s_systemInfos.find(typeHash) != s_systemInfos.end())
        return s_systemInfos[typeHash].Name;
    return "";
}

} // namespace Yogi
