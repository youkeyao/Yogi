#pragma once

#include <Yogi.h>

namespace Yogi
{

struct Field
{
    std::string   Name;
    std::size_t   Offset   = 0;
    uint32_t TypeHash = 0;
};

struct ComponentType
{
    std::string        Name;
    uint32_t      TypeHash = 0;
    std::vector<Field> Fields;
    std::size_t        Size = 0;
};

class ComponentManager
{
    typedef void* (*AddComponentFunc)(Entity&);
    typedef void (*RemoveComponentFunc)(Entity&);

public:
    static void Init();
    static void Clear();

    template <typename T>
    static void RegisterComponent(const std::vector<std::string>& fieldNames);

    static void EachComponentType(std::function<void(ComponentType&)> func);
    static void AddComponent(Entity& entity, uint32_t typeHash);
    static void RemoveComponent(Entity& entity, uint32_t typeHash);

    static ComponentType& GetComponentType(uint32_t typeHash);

private:
    static std::unordered_map<uint32_t, ComponentType>       s_componentTypes;
    static std::unordered_map<uint32_t, AddComponentFunc>    s_addComponentFuncs;
    static std::unordered_map<uint32_t, RemoveComponentFunc> s_removeComponentFuncs;
};

} // namespace Yogi