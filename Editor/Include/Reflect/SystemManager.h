#pragma once

#include <Yogi.h>

namespace Yogi
{

class SystemManager
{
    typedef void (*AddSystemFunc)(World&);
    typedef void (*RemoveSystemFunc)(World&);

    struct SystemInfo
    {
        std::string      Name;
        uint32_t         TypeHash;
        AddSystemFunc    AddFunc    = nullptr;
        RemoveSystemFunc RemoveFunc = nullptr;
    };

public:
    static void Init();
    static void Clear();

    template <typename T>
    static void RegisterSystem();

    static void EachSystemType(std::function<void(const std::string&, uint32_t)>&& func);
    static void AddSystem(World& world, uint32_t typeHash);
    static void RemoveSystem(World& world, uint32_t typeHash);

    static std::string GetSystemName(uint32_t typeHash);

private:
    static std::unordered_map<uint32_t, SystemInfo> s_systemInfos;
};

} // namespace Yogi